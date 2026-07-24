#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..");
const flowFile = path.join(
  root,
  "RaspberryPi",
  "NodeRED",
  "StepmotorSetup",
  "flows.json",
);
const outputs = {
  nodeRed: path.join(
    root,
    "RaspberryPi",
    "NodeRED",
    "StepmotorSetup",
    "TODO_NODE_RED.md",
  ),
  backplane: path.join(root, "Hardware", "PCB", "TODO_BACKPLANE_PCB.md"),
};

function normalize(text) {
  return text.trim().replace(/\s+/g, " ").toLocaleLowerCase("nb-NO");
}

function readChecked(file) {
  if (!fs.existsSync(file)) return new Map();
  const result = new Map();
  const linePattern = /^- \[([ xX])\] \*\*[A-Z]+-\d+\*\* (.+)$/gm;
  const content = fs.readFileSync(file, "utf8");
  for (const match of content.matchAll(linePattern)) {
    result.set(normalize(match[2]), match[1].toLowerCase() === "x");
  }
  return result;
}

function findTodoText(flows) {
  const notesTab = flows.find(
    (node) => node.type === "tab" && node.label?.trim().toLowerCase() === "notes",
  );
  if (!notesTab) throw new Error("Node-RED tab 'Notes' was not found");
  const todo = flows.find(
    (node) =>
      node.z === notesTab.id &&
      node.type === "comment" &&
      node.name?.trim().toLowerCase() === "todo",
  );
  if (!todo?.info?.trim()) throw new Error("Notes -> Todo comment is empty/missing");
  return { tabId: notesTab.id, commentId: todo.id, text: todo.info };
}

function splitSections(text) {
  const sections = { nodeRed: [], backplane: [] };
  let current = null;
  for (const rawLine of text.replace(/\r\n/g, "\n").split("\n")) {
    const line = rawLine.trim();
    if (!line || /^=+$/.test(line)) continue;
    if (/^node-?red\b/i.test(line)) {
      current = "nodeRed";
      continue;
    }
    if (/^backplane\b/i.test(line)) {
      current = "backplane";
      continue;
    }
    if (current) sections[current].push(line);
  }
  if (!sections.nodeRed.length || !sections.backplane.length) {
    throw new Error("Todo must contain non-empty Node-RED and Backplane sections");
  }
  return sections;
}

function render({ title, prefix, tasks, target, source }) {
  const previous = readChecked(target);
  const lines = tasks.map((task, index) => {
    const id = `${prefix}-${String(index + 1).padStart(3, "0")}`;
    const checked = previous.get(normalize(task)) ? "x" : " ";
    return `- [${checked}] **${id}** ${task}`;
  });
  const completed = lines.filter((line) => line.startsWith("- [x]")).length;
  return `# ${title}

> Synkronisert fra Node-RED-fanen \`Notes\`, kommentar \`Todo\`.
> Rediger oppgaveteksten i \`flows.json\`. Avkrysningsstatus kan oppdateres her
> og beholdes ved senere synk så lenge oppgaveteksten er den samme.

Kilde: \`${path.relative(root, flowFile).replaceAll("\\", "/")}\`  
Node-RED tab-ID: \`${source.tabId}\`  
Kommentar-ID: \`${source.commentId}\`  
Status: ${completed}/${tasks.length} fullført

## Oppgaver

${lines.join("\n")}
`;
}

function writeIfChanged(file, content) {
  const old = fs.existsSync(file) ? fs.readFileSync(file, "utf8") : null;
  if (old === content) return false;
  fs.mkdirSync(path.dirname(file), { recursive: true });
  fs.writeFileSync(file, content, "utf8");
  return true;
}

const flows = JSON.parse(fs.readFileSync(flowFile, "utf8").replace(/^\uFEFF/, ""));
const source = findTodoText(flows);
const sections = splitSections(source.text);
const changed = [];

if (
  writeIfChanged(
    outputs.nodeRed,
    render({
      title: "ToDo – Node-RED",
      prefix: "NR",
      tasks: sections.nodeRed,
      target: outputs.nodeRed,
      source,
    }),
  )
) changed.push(path.relative(root, outputs.nodeRed));

if (
  writeIfChanged(
    outputs.backplane,
    render({
      title: "ToDo – ny Backplane PCB-design",
      prefix: "BP",
      tasks: sections.backplane,
      target: outputs.backplane,
      source,
    }),
  )
) changed.push(path.relative(root, outputs.backplane));

console.log(
  JSON.stringify(
    {
      nodeRedTasks: sections.nodeRed.length,
      backplaneTasks: sections.backplane.length,
      changed,
    },
    null,
    2,
  ),
);
