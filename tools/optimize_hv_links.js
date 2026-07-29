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
const tabId = "eed4f5a48c9fc817";
const generatedPrefix = "hvbus_";

const hubs = [
  {
    key: "settings_save",
    target: "fn_panel_settings_persist",
    label: "Panel settings save",
  },
  {
    key: "settings_summary",
    target: "fn_settings_summary",
    label: "Settings summary update",
  },
  {
    key: "common_write_builder",
    target: "a0659fd7b52b27b9",
    label: "Common write builder",
  },
  {
    key: "driver_debounce",
    target: "fn_debounce_panel_motor_write",
    label: "Driver settings debounce",
  },
  {
    key: "sweep_engine",
    target: "fn_sweep_controller",
    label: "Sweep controller",
  },
  {
    key: "common_read",
    target: "7008312ed71aa5e5",
    label: "Common Modbus read",
  },
  {
    key: "common_write",
    target: "4ced50bdc5464de3",
    label: "Common Modbus write",
  },
];

let flows = JSON.parse(fs.readFileSync(flowPath, "utf8").replace(/^\uFEFF/, ""));
const byId = new Map(flows.map((node) => [node.id, node]));
const groupById = new Map(
  flows
    .filter((node) => node.type === "group" && node.z === tabId)
    .map((group) => [group.id, group]),
);

function safeId(text) {
  return text.replace(/[^a-zA-Z0-9_]/g, "_");
}

function addNodeToGroup(node, groupId) {
  const group = groupById.get(groupId);
  if (!group) throw new Error(`Group ${groupId} was not found`);
  node.g = groupId;
  if (!group.nodes.includes(node.id)) group.nodes.push(node.id);
}

function nextBusY(groupId, direction) {
  const group = groupById.get(groupId);
  const siblings = flows.filter(
    (node) =>
      node.z === tabId &&
      node.g === groupId &&
      node.id.startsWith(generatedPrefix) &&
      node._busDirection === direction,
  );
  return group.y + 65 + siblings.length * 42;
}

for (const hub of hubs) {
  const target = byId.get(hub.target);
  if (!target) throw new Error(`Hub target ${hub.target} was not found`);
  if (!target.g) throw new Error(`Hub target ${hub.target} is not grouped`);

  const linkInId = `${generatedPrefix}${hub.key}_in`;
  if (byId.has(linkInId)) continue;

  const sourcesByGroup = new Map();
  for (const source of flows) {
    if (
      source.z !== tabId ||
      source.type === "group" ||
      source.id.startsWith(generatedPrefix) ||
      !source.g ||
      source.g === target.g
    ) {
      continue;
    }
    for (let outputIndex = 0; outputIndex < (source.wires || []).length; outputIndex++) {
      const output = source.wires[outputIndex] || [];
      if (output.includes(hub.target)) {
        if (!sourcesByGroup.has(source.g)) sourcesByGroup.set(source.g, []);
        sourcesByGroup.get(source.g).push({ source, outputIndex });
      }
    }
  }

  if (!sourcesByGroup.size) continue;

  const linkIn = {
    id: linkInId,
    type: "link in",
    z: tabId,
    name: `BUS IN · ${hub.label}`,
    links: [],
    x: groupById.get(target.g).x + 45,
    y: nextBusY(target.g, "in"),
    wires: [[hub.target]],
    _busDirection: "in",
  };
  addNodeToGroup(linkIn, target.g);
  flows.push(linkIn);
  byId.set(linkIn.id, linkIn);

  for (const [sourceGroupId, connections] of sourcesByGroup) {
    const linkOutId =
      `${generatedPrefix}${hub.key}_out_` + safeId(sourceGroupId);
    const sourceGroup = groupById.get(sourceGroupId);
    const linkOut = {
      id: linkOutId,
      type: "link out",
      z: tabId,
      name: `BUS OUT · ${hub.label}`,
      mode: "link",
      links: [linkInId],
      x: sourceGroup.x + sourceGroup.w - 35,
      y: nextBusY(sourceGroupId, "out"),
      wires: [],
      _busDirection: "out",
    };
    addNodeToGroup(linkOut, sourceGroupId);
    flows.push(linkOut);
    byId.set(linkOut.id, linkOut);
    linkIn.links.push(linkOutId);

    for (const { source, outputIndex } of connections) {
      const output = source.wires[outputIndex];
      source.wires[outputIndex] = output.map((targetId) =>
        targetId === hub.target ? linkOutId : targetId,
      );
    }
  }
}

// `_busDirection` is only used while calculating positions.
for (const node of flows) {
  if (node.id && node.id.startsWith(generatedPrefix)) {
    delete node._busDirection;
  }
}

fs.writeFileSync(flowPath, JSON.stringify(flows));

const generated = flows.filter((node) => node.id.startsWith(generatedPrefix));
console.log(
  `Created ${generated.length} local Link In/Out bus nodes for ${hubs.length} shared targets`,
);
