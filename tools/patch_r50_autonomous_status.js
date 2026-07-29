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

function addNode(node) {
  if (byId.has(node.id)) return requireNode(node.id);
  flows.push(node);
  byId.set(node.id, node);
  if (node.g) {
    const group = requireNode(node.g);
    if (!group.nodes.includes(node.id)) group.nodes.push(node.id);
  }
  return node;
}

const parsePosition = requireNode("663f37b7736327fa");
parsePosition.outputs = 6;
parsePosition.wires ||= [];
while (parsePosition.wires.length < 6) parsePosition.wires.push([]);
if (!parsePosition.wires[5].includes("fn_r50_position_status_trigger")) {
  parsePosition.wires[5].push("fn_r50_position_status_trigger");
}

parsePosition.func = parsePosition.func
  .replace(
    `        { sweepPosition: true, axis, angle, raw: raw32 }
    ];`,
    `        { sweepPosition: true, axis, angle, raw: raw32 },
        { r50PositionSample: true, axis, angle, raw: raw32 }
    ];`,
  )
  .replace(
    `        { sweepPosition: true, axis, angle, raw: raw32 }
    ];`,
    `        { sweepPosition: true, axis, angle, raw: raw32 },
        { r50PositionSample: true, axis, angle, raw: raw32 }
    ];`,
  );
if ((parsePosition.func.match(/r50PositionSample/g) || []).length !== 2) {
  throw new Error("Could not add both vertical and horizontal R50 position samples");
}

const statusTrigger = addNode({
  id: "fn_r50_position_status_trigger",
  type: "function",
  z: "eed4f5a48c9fc817",
  g: "grp_hv_polling",
  name: "Position change → R50 Status 07 (max 5 Hz)",
  func: `if (!msg.r50PositionSample) return null;

const MIN_INTERVAL_MS = 200;

function encodedState() {
    const horizontal = Number(global.get('horizontal_angle_actual'));
    const vertical = Number(global.get('vertical_angle_actual'));
    return {
        h: Number.isFinite(horizontal) ? Math.round(horizontal) : 999,
        v: Number.isFinite(vertical) ? Math.round(vertical) : 999
    };
}
function stateKey(state) {
    return state.h + ':' + state.v;
}
function statusMessage(reason) {
    return {
        r50Reply: 'status',
        r50Trigger: reason,
        payload: reason
    };
}

const now = Date.now();
const current = encodedState();
const currentKey = stateKey(current);
const lastKey = context.get('lastSentKey');
const lastAt = Number(context.get('lastSentAt')) || 0;

if (currentKey === lastKey) return null;

if ((now - lastAt) >= MIN_INTERVAL_MS) {
    context.set('lastSentKey', currentKey);
    context.set('lastSentAt', now);
    context.set('pendingKey', null);
    return statusMessage('position-change');
}

context.set('pendingKey', currentKey);
if (!context.get('pendingTimer')) {
    const remaining = Math.max(1, MIN_INTERVAL_MS - (now - lastAt));
    const timer = setTimeout(() => {
        context.set('pendingTimer', null);
        const latest = encodedState();
        const latestKey = stateKey(latest);
        if (latestKey === context.get('lastSentKey')) return;
        context.set('lastSentKey', latestKey);
        context.set('lastSentAt', Date.now());
        context.set('pendingKey', null);
        node.send(statusMessage('position-change-trailing'));
    }, remaining);
    context.set('pendingTimer', timer);
}
return null;`,
  outputs: 1,
  timeout: 0,
  noerr: 0,
  initialize: "",
  finalize: `const timer = context.get('pendingTimer');
if (timer) clearTimeout(timer);`,
  libs: [],
  x: 410,
  y: 755,
  wires: [["link_out_r50_position_status"]],
});

const autonomousStatusIn = requireNode("link_in_r50_lamp_feedback");
autonomousStatusIn.name = "R50 autonomous status trigger";

const positionLinkOut = addNode({
  id: "link_out_r50_position_status",
  type: "link out",
  z: "eed4f5a48c9fc817",
  g: "grp_hv_polling",
  name: "Position change → R50 Status",
  mode: "link",
  links: [autonomousStatusIn.id],
  x: 905,
  y: 755,
  wires: [],
});
if (!autonomousStatusIn.links.includes(positionLinkOut.id)) {
  autonomousStatusIn.links.push(positionLinkOut.id);
}

const lampStatus = requireNode("fn_backplane_lamp_status");
lampStatus.func = `const newState = msg.faValue ? 1 : 0;
const previousState = global.get('backplane_lamp_state');
const changed = previousState === undefined ||
    Number(previousState) !== newState;

global.set('backplane_lamp_state', newState);
const status = {
    ...msg,
    payload: newState ? 'På (FA=1)' : 'Av (FA=0)'
};
return [
    status,
    changed ? {
        r50Reply: 'status',
        r50Trigger: 'lamp-change',
        payload: 'lamp-change'
    } : null
];`;

const lampLinkOut = requireNode("link_out_r50_lamp_feedback");
lampLinkOut.name = "Lamp change → R50 Status";

fs.writeFileSync(flowPath, JSON.stringify(flows));
console.log(
  "Added autonomous R50 Status 07 on position change (max 5 Hz) and lamp change",
);
