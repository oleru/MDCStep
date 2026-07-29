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
const prefix = "hvroute_";
const longDistance = 900;

const flows = JSON.parse(fs.readFileSync(flowPath, "utf8").replace(/^\uFEFF/, ""));

const byId = new Map(flows.map((node) => [node.id, node]));
const groups = flows.filter(
  (node) => node.type === "group" && node.z === tabId,
);
const groupById = new Map(groups.map((group) => [group.id, group]));

function compact(text, fallback) {
  const value = String(text || fallback)
    .replace(/^BUS (IN|OUT) · /, "")
    .replace(/\s+/g, " ")
    .trim();
  return value.length > 30 ? `${value.slice(0, 27)}…` : value;
}

function safeId(text) {
  return text.replace(/[^a-zA-Z0-9_]/g, "_");
}

const routes = new Map();
for (const source of flows) {
  if (
    source.z !== tabId ||
    source.type === "group" ||
    source.type === "link in" ||
    source.type === "link out" ||
    !source.g
  ) {
    continue;
  }
  for (let outputIndex = 0; outputIndex < (source.wires || []).length; outputIndex++) {
    for (const targetId of source.wires[outputIndex] || []) {
      const target = byId.get(targetId);
      if (!target || target.z !== tabId || !target.g || target.g === source.g) {
        continue;
      }
      const distance = Math.hypot(
        (source.x || 0) - (target.x || 0),
        (source.y || 0) - (target.y || 0),
      );
      if (distance <= longDistance) continue;

      const key = `${source.id}_${outputIndex}_${target.g}`;
      if (!routes.has(key)) {
        routes.set(key, { source, outputIndex, targetGroupId: target.g, targets: [] });
      }
      routes.get(key).targets.push(targetId);
    }
  }
}

const generated = [];
let routeNumber = 0;
for (const route of routes.values()) {
  routeNumber += 1;
  const sourceGroup = groupById.get(route.source.g);
  const targetGroup = groupById.get(route.targetGroupId);
  if (!sourceGroup || !targetGroup) {
    throw new Error("A route references a missing visual group");
  }

  const routeKey = safeId(
    `${routeNumber}_${route.source.id}_${route.outputIndex}_${route.targetGroupId}`,
  );
  const outId = `${prefix}${routeKey}_out`;
  const inId = `${prefix}${routeKey}_in`;
  const sourceLabel = compact(route.source.name, route.source.type);
  const targetLabel =
    route.targets.length === 1
      ? compact(byId.get(route.targets[0]).name, byId.get(route.targets[0]).type)
      : `${route.targets.length} targets`;

  const linkOut = {
    id: outId,
    type: "link out",
    z: tabId,
    g: route.source.g,
    name: `LONG OUT · ${sourceLabel}`,
    mode: "link",
    links: [inId],
    x: 0,
    y: 0,
    wires: [],
  };
  const linkIn = {
    id: inId,
    type: "link in",
    z: tabId,
    g: route.targetGroupId,
    name: `LONG IN · ${targetLabel}`,
    links: [outId],
    x: 0,
    y: 0,
    wires: [route.targets.slice()],
  };

  sourceGroup.nodes.push(outId);
  targetGroup.nodes.push(inId);
  generated.push(linkOut, linkIn);
  byId.set(outId, linkOut);
  byId.set(inId, linkIn);

  const targetSet = new Set(route.targets);
  const originalOutput = route.source.wires[route.outputIndex] || [];
  const replacement = [];
  let inserted = false;
  for (const targetId of originalOutput) {
    if (targetSet.has(targetId)) {
      if (!inserted) {
        replacement.push(outId);
        inserted = true;
      }
    } else {
      replacement.push(targetId);
    }
  }
  route.source.wires[route.outputIndex] = replacement;
}

flows.push(...generated);

// Put helper nodes on dedicated margins inside each group. Expand groups if needed.
for (const group of groups) {
  const inputs = flows.filter(
    (node) => node.g === group.id && node.id && node.id.startsWith(prefix) && node.type === "link in",
  );
  const outputs = flows.filter(
    (node) => node.g === group.id && node.id && node.id.startsWith(prefix) && node.type === "link out",
  );
  const requiredHeight = 100 + Math.max(inputs.length, outputs.length) * 42;
  group.h = Math.max(group.h, requiredHeight);
  inputs.forEach((node, index) => {
    node.x = group.x + 35;
    node.y = group.y + 65 + index * 42;
  });
  outputs.forEach((node, index) => {
    node.x = group.x + group.w - 35;
    node.y = group.y + 65 + index * 42;
  });
}

// Expanding one group must move groups below it, including all member nodes.
const columns = new Map();
for (const group of groups) {
  if (!columns.has(group.x)) columns.set(group.x, []);
  columns.get(group.x).push(group);
}
for (const columnGroups of columns.values()) {
  columnGroups.sort((a, b) => a.y - b.y);
  let nextY = 40;
  for (const group of columnGroups) {
    const deltaY = nextY - group.y;
    if (deltaY) {
      group.y += deltaY;
      for (const nodeId of group.nodes) {
        const node = byId.get(nodeId);
        if (node) node.y += deltaY;
      }
    }
    nextY = group.y + group.h + 40;
  }
}

fs.writeFileSync(flowPath, JSON.stringify(flows));
console.log(
  `Replaced ${routes.size} long cross-group routes with ${generated.length} Link In/Out nodes`,
);
