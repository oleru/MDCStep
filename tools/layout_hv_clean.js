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

const flows = JSON.parse(fs.readFileSync(flowPath, "utf8").replace(/^\uFEFF/, ""));
const tab = flows.find((node) => node.id === tabId);
if (!tab) {
  throw new Error(`Flow tab ${tabId} was not found`);
}

const tabNodes = flows.filter(
  (node) => node.z === tabId && node.type !== "group",
);
const byId = new Map(tabNodes.map((node) => [node.id, node]));

const groups = [
  {
    id: "grp_hv_config_state",
    name: "1. Configuration, activation and position feedback",
    column: 0,
    color: "#DCEBFA",
    ids: [
      "2dd4a5c79fe3ce93",
      "64bf0876a4df75f2",
      "524619251c51f72a",
      "f32c4777c959aee0",
      "9ef55ddb16f26979",
      "e7455820d1551c37",
      "594660874057498f",
      "8e5828111f28709a",
      "a20276286cdf048a",
      "c5a1b43d7f89bccc",
      "7497d8966788dc66",
      "b9e613a47f74614f",
      "a6e1964891f3652b",
      "ccf4be0982be3ba7",
      "7eef7646a40f2f0f",
      "0b9e82eefe037137",
      "6a54d233fb6d6ee6",
    ],
  },
  {
    id: "grp_hv_polling",
    name: "2. Position polling and common read",
    column: 0,
    color: "#E3F2FD",
    ids: [
      "43312165bf9019c5",
      "47d3ddbaf12bf16e",
      "8e3d3557811ca981",
      "bc21209c6d8bb6c9",
      "c05bc2615e4aa47d",
      "7008312ed71aa5e5",
      "663f37b7736327fa",
      "bf559f080c85501d",
    ],
  },
  {
    id: "grp_hv_vertical_manual",
    name: "3. Vertical manual control",
    column: 0,
    color: "#E8F5E9",
    ids: [
      "15c15d32f494028b",
      "2e7c87248e2d2e9c",
      "8c90afaf437d28c4",
      "c6a9a2fb64cd1816",
      "2e5ee353936d8fc8",
      "40e09e08cf894c08",
      "7d2c7359f3bad84b",
      "a741d977ee7dd99a",
      "ce6e1abaca34c8fc",
    ],
  },
  {
    id: "grp_hv_horizontal_manual",
    name: "4. Horizontal manual control",
    column: 0,
    color: "#E8F5E9",
    ids: [
      "e8a90372c1c6737d",
      "2513e077aea163c1",
      "d05110962493b889",
      "88f567d37c944ad6",
      "31b8faf9b88354ee",
      "dded94eeb8da8297",
      "b070e1d9e536415c",
      "1648a6ed3351e531",
      "e29f5d1f055707bb",
    ],
  },
  {
    id: "grp_hv_software_limits",
    name: "5. Software limits",
    column: 1,
    color: "#FFF3E0",
    ids: [
      "ui_swlim_h_min",
      "chg_swlim_h_min",
      "ui_swlim_h_max",
      "chg_swlim_h_max",
      "ui_swlim_v_min",
      "chg_swlim_v_min",
      "ui_swlim_v_max",
      "chg_swlim_v_max",
      "btn_swlim_calc_h",
      "btn_swlim_write_h",
      "btn_swlim_calc_v",
      "btn_swlim_write_v",
      "fn_sw_limits_build",
      "txt_swlim_status",
    ],
  },
  {
    id: "grp_hv_motion_profile",
    name: "6. Motion profile",
    column: 1,
    color: "#FFF3E0",
    ids: [
      "ui_motion_start_speed",
      "chg_motion_start_speed",
      "ui_motion_stop_speed",
      "chg_motion_stop_speed",
      "ui_motion_accel_time",
      "chg_motion_accel_time",
      "ui_motion_decel_time",
      "chg_motion_decel_time",
      "btn_motion_calc",
      "btn_motion_write_v",
      "btn_motion_write_h",
      "btn_motion_write_both",
      "fn_motion_profile_build",
      "txt_motion_status",
    ],
  },
  {
    id: "grp_hv_goto_vertical",
    name: "7. Vertical Goto",
    column: 1,
    color: "#F3E5F5",
    ids: [
      "ui_goto_v_angle",
      "btn_goto_v_angle",
      "chg_goto_v_angle",
      "txt_goto_v_status",
    ],
  },
  {
    id: "grp_hv_goto_horizontal",
    name: "8. Horizontal Goto and legacy position tools",
    column: 1,
    color: "#F3E5F5",
    ids: [
      "ui_goto_h_angle",
      "btn_goto_h_angle",
      "chg_goto_h_angle",
      "txt_goto_h_status",
      "c71635b992319bcf",
      "6fc50b210b799c08",
      "7ceb88f85429fb3b",
      "d256a611ea362da5",
      "53ab8ab94349041b",
      "dddeea790f7a09a4",
      "bd9e370597e4174a",
      "fn_goto_angle_build",
      "646ebddec9574fd3",
      "cab9cc7c98681e98",
      "2b6c732f8ebed2f0",
    ],
  },
  {
    id: "grp_hv_sweep_horizontal",
    name: "9. Horizontal Sweep",
    column: 2,
    color: "#E0F2F1",
    ids: [
      "ui_sweep_h_start",
      "chg_sweep_h_start",
      "ui_sweep_h_end",
      "chg_sweep_h_end",
      "btn_sweep_h_on",
      "btn_sweep_h_off",
      "txt_sweep_h_status",
    ],
  },
  {
    id: "grp_hv_sweep_vertical",
    name: "10. Vertical Sweep",
    column: 2,
    color: "#E0F2F1",
    ids: [
      "ui_sweep_v_start",
      "chg_sweep_v_start",
      "ui_sweep_v_end",
      "chg_sweep_v_end",
      "btn_sweep_v_on",
      "btn_sweep_v_off",
      "txt_sweep_v_status",
    ],
  },
  {
    id: "grp_hv_sweep_engine",
    name: "11. Shared Sweep engine",
    column: 2,
    color: "#D7CCC8",
    ids: ["fn_sweep_controller"],
  },
  {
    id: "grp_hv_settings_summary",
    name: "12. Settings summary",
    column: 2,
    color: "#ECEFF1",
    ids: [
      "inj_settings_summary_init",
      "fn_settings_summary",
      "tpl_settings_summary",
    ],
  },
  {
    id: "grp_hv_common_write",
    name: "13. Common Modbus write queue",
    column: 2,
    color: "#FFEBEE",
    ids: [
      "link_in_r50_motor",
      "a0659fd7b52b27b9",
      "4ced50bdc5464de3",
      "c9ec82e12e0d2c08",
    ],
  },
  {
    id: "grp_hv_settings_storage",
    name: "14. Panel settings file – load, debounce and save",
    column: 3,
    color: "#F1F8E9",
    ids: [
      "inj_panel_settings_load",
      "fn_panel_settings_filename",
      "panel_settings_file_in",
      "fn_restore_panel_settings",
      "inj_panel_settings_fallback",
      "fn_panel_settings_persist",
      "panel_settings_file_out",
      "fn_panel_settings_saved",
      "catch_panel_settings_file",
      "fn_panel_settings_file_error",
      "btn_panel_settings_save_now",
      "fn_mark_force_panel_settings_save",
      "btn_panel_settings_reload",
      "txt_panel_persistence_status",
    ],
  },
  {
    id: "grp_hv_driver_sync",
    name: "15. Driver startup write, readback and verification",
    column: 3,
    color: "#FFF8E1",
    ids: [
      "inj_panel_motor_settings_read",
      "fn_build_panel_motor_reads",
      "fn_parse_panel_motor_settings",
      "inj_panel_limits_write_startup",
      "fn_panel_limits_write_startup",
      "fn_build_versioned_run_speed_write",
      "fn_set_panel_rated_current",
      "inj_panel_settings_verify",
      "fn_build_panel_settings_verify_reads",
      "fn_debounce_panel_motor_write",
    ],
  },
];

const claimed = new Map();
for (const group of groups) {
  for (const id of group.ids) {
    if (!byId.has(id)) {
      throw new Error(`Node ${id} in group "${group.name}" was not found`);
    }
    if (claimed.has(id)) {
      throw new Error(
        `Node ${id} is assigned to both "${claimed.get(id)}" and "${group.name}"`,
      );
    }
    claimed.set(id, group.name);
  }
}

const unclaimed = tabNodes.filter((node) => !claimed.has(node.id));
if (unclaimed.length) {
  groups.push({
    id: "grp_hv_misc",
    name: "16. Utilities and uncategorized nodes",
    column: 3,
    color: "#EEEEEE",
    ids: unclaimed.map((node) => node.id),
  });
}

const sourceTypes = new Set([
  "inject",
  "ui-button",
  "ui-number-input",
  "ui-slider",
  "catch",
  "link in",
]);
const sinkTypes = new Set([
  "ui-text",
  "debug",
  "ui-template",
  "modbus-response",
]);
const ioTypes = new Set([
  "modbus-flex-getter",
  "modbus-flex-write",
  "file",
  "file in",
]);

function internalPredecessors(groupIds) {
  const predecessors = new Map(groupIds.map((id) => [id, []]));
  const idSet = new Set(groupIds);
  for (const id of groupIds) {
    const node = byId.get(id);
    for (const output of node.wires || []) {
      for (const targetId of output || []) {
        if (idSet.has(targetId)) {
          predecessors.get(targetId).push(id);
        }
      }
    }
  }
  return predecessors;
}

function levelFor(node, predecessors) {
  if (sourceTypes.has(node.type)) return 0;
  if (sinkTypes.has(node.type)) return 3;
  if (ioTypes.has(node.type)) return 2;
  const preds = predecessors.get(node.id) || [];
  if (preds.some((id) => sourceTypes.has(byId.get(id).type))) return 1;
  if (preds.length) return 2;
  return 1;
}

const groupWidth = 900;
const groupGapX = 40;
const groupGapY = 40;
const columnX = [40, 980, 1920, 2860];
const columnBottom = [40, 40, 40, 40];

for (const group of groups) {
  const nodes = group.ids.map((id) => byId.get(id));
  const predecessors = internalPredecessors(group.ids);
  const lanes = [[], [], [], []];

  for (const node of nodes) {
    lanes[levelFor(node, predecessors)].push(node);
  }
  for (const lane of lanes) {
    lane.sort((a, b) => (a.y || 0) - (b.y || 0) || (a.x || 0) - (b.x || 0));
  }

  const maxLaneLength = Math.max(1, ...lanes.map((lane) => lane.length));
  const groupHeight = 90 + maxLaneLength * 52;
  const x = columnX[group.column];
  const y = columnBottom[group.column];

  group.x = x;
  group.y = y;
  group.w = groupWidth;
  group.h = groupHeight;
  columnBottom[group.column] += groupHeight + groupGapY;

  const laneX = [x + 150, x + 370, x + 590, x + 790];
  for (let laneIndex = 0; laneIndex < lanes.length; laneIndex++) {
    lanes[laneIndex].forEach((node, rowIndex) => {
      node.x = laneX[laneIndex];
      node.y = y + 65 + rowIndex * 52;
      node.g = group.id;
    });
  }
}

const oldGroupIds = new Set(
  flows
    .filter((node) => node.type === "group" && node.z === tabId)
    .map((node) => node.id),
);
const retained = flows.filter((node) => !oldGroupIds.has(node.id));
const newGroupNodes = groups.map((group) => ({
  id: group.id,
  type: "group",
  z: tabId,
  name: group.name,
  style: {
    stroke: "#7f8c8d",
    fill: group.color,
    label: true,
    color: "#2c3e50",
  },
  nodes: group.ids,
  x: group.x,
  y: group.y,
  w: group.w,
  h: group.h,
}));

const output = [...retained, ...newGroupNodes];
fs.writeFileSync(flowPath, JSON.stringify(output));

const maxX = Math.max(...newGroupNodes.map((group) => group.x + group.w));
const maxY = Math.max(...newGroupNodes.map((group) => group.y + group.h));
console.log(
  `Laid out ${tabNodes.length} nodes in ${newGroupNodes.length} groups; bounds ${maxX}x${maxY}`,
);
if (unclaimed.length) {
  console.log(
    `Placed ${unclaimed.length} unclassified nodes in "${groups.at(-1).name}"`,
  );
}
