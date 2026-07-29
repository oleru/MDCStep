#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");

const flowPath = path.resolve(
  __dirname,
  "..",
  "RaspberryPi",
  "NodeRED",
  "StepmotorSetup",
  "flows.json",
);
const flows = JSON.parse(fs.readFileSync(flowPath, "utf8").replace(/^\uFEFF/, ""));
const byId = new Map(flows.map((node) => [node.id, node]));

function requireNode(id) {
  const node = byId.get(id);
  if (!node) throw new Error(`Required node ${id} was not found`);
  return node;
}

function replaceOnce(text, before, after, label) {
  if (text.includes(after)) return text;
  const count = text.split(before).length - 1;
  if (count !== 1) {
    throw new Error(`${label}: expected one replacement location, found ${count}`);
  }
  return text.replace(before, after);
}

function addWire(sourceId, outputIndex, targetId) {
  const source = requireNode(sourceId);
  source.wires ||= [];
  while (source.wires.length <= outputIndex) source.wires.push([]);
  if (!source.wires[outputIndex].includes(targetId)) {
    source.wires[outputIndex].push(targetId);
  }
}

function addToGroup(groupId, nodeId) {
  const group = requireNode(groupId);
  group.nodes ||= [];
  if (!group.nodes.includes(nodeId)) group.nodes.push(nodeId);
}

function addNode(node) {
  if (byId.has(node.id)) return requireNode(node.id);
  flows.push(node);
  byId.set(node.id, node);
  if (node.g) addToGroup(node.g, node.id);
  return node;
}

const sweep = requireNode("fn_sweep_controller");
sweep.func = replaceOnce(
  sweep.func,
  `    if (action === 'stop') {
        state[axis] = { enabled: false };
        context.set('sweep_state', state);
        node.status({ fill: 'grey', shape: 'ring', text: axis + ' sweep off' });
        return outputs(axis, stopWrite(axis), 'Sweep OFF - controlled stop');
    }

    if (action !== 'start') return null;`,
  `    if (action === 'takeover') {
        const wasEnabled = Boolean(state[axis] && state[axis].enabled);
        state[axis] = { enabled: false };
        context.set('sweep_state', state);
        if (!wasEnabled) return null;
        node.status({ fill: 'yellow', shape: 'ring',
            text: axis + ' sweep stopped by joystick' });
        return outputs(axis, null,
            'Sweep stopped: joystick takeover (' +
            (control.source || 'manual') + ')');
    }

    if (action === 'stop') {
        state[axis] = { enabled: false };
        context.set('sweep_state', state);
        node.status({ fill: 'grey', shape: 'ring', text: axis + ' sweep off' });
        return outputs(axis, stopWrite(axis), 'Sweep OFF - controlled stop');
    }

    if (action !== 'start') return null;`,
  "Add non-stopping Sweep takeover action",
);

const r50 = requireNode("fn_r50_handle_command");
r50.func = replaceOnce(
  r50.func,
  `function stopAndRestore() {
    return [
        motorMessage('horizontal', 0x00C8, 0),
        motorMessage('vertical', 0x00C8, 0),
        motorMessage('horizontal', 0x009A, configuredSpeed('horizontal')),
        motorMessage('vertical', 0x009A, configuredSpeed('vertical'))
    ];
}`,
  `function stopAndRestore(activeState) {
    const messages = [];
    for (const axis of ['horizontal', 'vertical']) {
        const axisState = activeState && activeState[axis];
        if (!axisState || axisState.input === 0) continue;
        messages.push(motorMessage(axis, 0x00C8, 0));
        messages.push(motorMessage(axis, 0x009A, configuredSpeed(axis)));
    }
    return messages;
}`,
  "Make the R50 watchdog axis-selective",
);
r50.func = replaceOnce(
  r50.func,
  `const oldTimer = context.get('watchdogTimer');
if (oldTimer) clearTimeout(oldTimer);
const watchdogTimer = setTimeout(() => {
    global.set('r50_control_active', false);
    context.set('motionState', {
        horizontal: { input: 0, speed: null, direction: null },
        vertical: { input: 0, speed: null, direction: null }
    });
    node.send([
        stopAndRestore(),
        null,
        null,
        { payload: 'R50 TIMEOUT >350 ms: H/V stopped' }
    ]);
}, 350);
context.set('watchdogTimer', watchdogTimer);`,
  `const oldTimer = context.get('watchdogTimer');
if (oldTimer) clearTimeout(oldTimer);
context.set('watchdogTimer', null);

// Neutral R50 telegrams are presence/keepalive only. They must never stop
// an active Goto or Sweep. Arm the watchdog only while R50 moves H or V.
if (horizontal !== 0 || vertical !== 0) {
    const watchdogTimer = setTimeout(() => {
        const activeState = context.get('motionState') || {};
        const stopMessages = stopAndRestore(activeState);
        global.set('r50_control_active', false);
        context.set('motionState', {
            horizontal: { input: 0, speed: null, direction: null },
            vertical: { input: 0, speed: null, direction: null }
        });
        context.set('watchdogTimer', null);
        if (stopMessages.length) {
            node.send([
                stopMessages,
                null,
                null,
                { payload: 'R50 TIMEOUT >350 ms: active R50 axes stopped' }
            ]);
        }
    }, 350);
    context.set('watchdogTimer', watchdogTimer);
}`,
  "Do not arm R50 watchdog for neutral telegrams",
);

const sweepBusIn = requireNode("hvbus_sweep_engine_in");

const r50Takeover = addNode({
  id: "fn_r50_sweep_takeover",
  type: "function",
  z: "tab_r50_protocol",
  name: "Non-zero R50 joystick → stop Sweep",
  func: `const telegram = msg.r50;
if (!telegram || telegram.type !== 0x08 || !Array.isArray(telegram.bytes)) {
    return null;
}
const bytes = telegram.bytes;
if (bytes.length !== 8) return null;
const searchlightId = Number(env.get('R50_SEARCHLIGHT_ID') || 1);
if (bytes[3] !== searchlightId) return null;
function int8(value) { return value > 127 ? value - 256 : value; }
const horizontal = int8(bytes[4]);
const vertical = int8(bytes[5]);
const messages = [];
if (horizontal !== 0) {
    messages.push({
        sweepControl: true,
        axis: 'horizontal',
        action: 'takeover',
        source: 'R50 joystick'
    });
}
if (vertical !== 0) {
    messages.push({
        sweepControl: true,
        axis: 'vertical',
        action: 'takeover',
        source: 'R50 joystick'
    });
}
return messages.length ? [messages] : null;`,
  outputs: 1,
  timeout: 0,
  noerr: 0,
  initialize: "",
  finalize: "",
  libs: [],
  x: 720,
  y: 320,
  wires: [["link_out_r50_sweep_takeover"]],
});
const r50TakeoverOut = addNode({
  id: "link_out_r50_sweep_takeover",
  type: "link out",
  z: "tab_r50_protocol",
  name: "R50 joystick takeover → Sweep",
  mode: "link",
  links: [sweepBusIn.id],
  x: 975,
  y: 320,
  wires: [],
});
addWire("fn_r50_validate_decode", 0, r50Takeover.id);
if (!sweepBusIn.links.includes(r50TakeoverOut.id)) {
  sweepBusIn.links.push(r50TakeoverOut.id);
}

function addManualTakeover(axis, groupId, buttonIds, x, y) {
  const suffix = axis === "horizontal" ? "h" : "v";
  const functionId = `fn_local_${suffix}_sweep_takeover`;
  const linkOutId = `link_out_local_${suffix}_sweep_takeover`;
  const takeover = addNode({
    id: functionId,
    type: "function",
    z: "eed4f5a48c9fc817",
    g: groupId,
    name: `${axis} manual input → stop Sweep`,
    func: `return {
    sweepControl: true,
    axis: '${axis}',
    action: 'takeover',
    source: 'local direction control'
};`,
    outputs: 1,
    timeout: 0,
    noerr: 0,
    initialize: "",
    finalize: "",
    libs: [],
    x,
    y,
    wires: [[linkOutId]],
  });
  const linkOut = addNode({
    id: linkOutId,
    type: "link out",
    z: "eed4f5a48c9fc817",
    g: groupId,
    name: `${axis} takeover → Sweep`,
    mode: "link",
    links: [sweepBusIn.id],
    x: x + 245,
    y,
    wires: [],
  });
  for (const buttonId of buttonIds) addWire(buttonId, 0, takeover.id);
  if (!sweepBusIn.links.includes(linkOut.id)) sweepBusIn.links.push(linkOut.id);
}

addManualTakeover(
  "vertical",
  "grp_hv_vertical_manual",
  ["15c15d32f494028b", "8c90afaf437d28c4"],
  590,
  1260,
);
addManualTakeover(
  "horizontal",
  "grp_hv_horizontal_manual",
  ["e8a90372c1c6737d", "d05110962493b889"],
  590,
  1754,
);

const split = requireNode("8e7b5442d6895908");
split.func = `const x = (msg.payload && typeof msg.payload.x === 'number') ? msg.payload.x : 0;
const y = (msg.payload && typeof msg.payload.y === 'number') ? msg.payload.y : 0;

const src = msg.selectedSource || msg.source || 'unknown';

node.status({fill:'green', shape:'dot', text:\`\${src} x:\${x} y:\${y}\`});

const sweepTakeover = [];
if (src === 'joystick' && x !== 0) {
  sweepTakeover.push({
    sweepControl:true, axis:'horizontal', action:'takeover',
    source:'local joystick'
  });
}
if (src === 'joystick' && y !== 0) {
  sweepTakeover.push({
    sweepControl:true, axis:'vertical', action:'takeover',
    source:'local joystick'
  });
}

return [
  { topic:'joy/x', payload:x, source:src, owner:msg.owner, session:msg.session, reason:msg.reason },
  { topic:'joy/y', payload:y, source:src, owner:msg.owner, session:msg.session, reason:msg.reason },
  sweepTakeover.length ? sweepTakeover : null
];`;
split.outputs = 3;
split.wires ||= [];
while (split.wires.length < 3) split.wires.push([]);
if (!split.wires[2].includes("link_out_local_joystick_sweep_takeover")) {
  split.wires[2].push("link_out_local_joystick_sweep_takeover");
}

const joystickGroup = requireNode("b62f2924c82632d3");
const joystickTakeoverOut = addNode({
  id: "link_out_local_joystick_sweep_takeover",
  type: "link out",
  z: "c1d1c8877083d891",
  g: joystickGroup.id,
  name: "Local joystick takeover → Sweep",
  mode: "link",
  links: [sweepBusIn.id],
  x: 1000,
  y: 440,
  wires: [],
});
if (!sweepBusIn.links.includes(joystickTakeoverOut.id)) {
  sweepBusIn.links.push(joystickTakeoverOut.id);
}

fs.writeFileSync(flowPath, JSON.stringify(flows));
console.log("Patched neutral-safe R50 watchdog and local/external joystick Sweep takeover");
