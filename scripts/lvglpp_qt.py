#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Standalone Python implementation of the lvglpp Qt helper workflows.

This utility mirrors the `rlvgl-creator qt` command family for structural
inspection and scripting. It intentionally uses only the Python standard
library so it can run before the Rust workspace has been built.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable


QT_IR_VERSION = 2
QT_EMIT_VERSION_DATA = 1
QT_EMIT_VERSION_RLVGL = 22
QT_EXTERNALS_VERSION = 1
QT_IR_SCHEMA_ID = "https://rlvgl.dev/schemas/qt-ir.schema.json"
QT_IR_SCHEMA_COMMENT = (
    "schemas/qt-ir.schema.json - rlvgl-creator qt-ir IR. Regenerate with "
    "`rlvgl-creator qt schema --out schemas/qt-ir.schema.json`. See "
    "docs/qt-support/02-ir-schema.md for the bumping policy."
)


class QtError(RuntimeError):
    """User-facing Qt utility error."""


@dataclass
class UiImport:
    module: str
    version: str | None = None
    alias: str | None = None

    def to_json(self) -> dict[str, Any]:
        return {"module": self.module, "version": self.version, "alias": self.alias}


@dataclass
class UiProperty:
    name: str
    ty: str
    default_value: str | None
    readonly: bool = False
    default_kw: bool = False

    def to_json(self) -> dict[str, Any]:
        out: dict[str, Any] = {
            "name": self.name,
            "ty": self.ty,
            "readonly": self.readonly,
            "default_kw": self.default_kw,
        }
        if self.default_value is not None:
            out["default_value"] = self.default_value
        return out


@dataclass
class UiSignalParam:
    name: str
    ty: str

    def to_json(self) -> dict[str, Any]:
        return {"name": self.name, "ty": self.ty}


@dataclass
class UiSignal:
    name: str
    params: list[UiSignalParam] = field(default_factory=list)

    def to_json(self) -> dict[str, Any]:
        return {"name": self.name, "params": [p.to_json() for p in self.params]}


@dataclass
class UiHandler:
    signal: str
    body: str

    def to_json(self) -> dict[str, Any]:
        return {"signal": self.signal, "body": self.body}


@dataclass
class UiAssignmentValue:
    kind: str
    text: str | None = None
    item: "UiItem | None" = None
    items: list["UiAssignmentValue"] | None = None

    @staticmethod
    def expression(text: str) -> "UiAssignmentValue":
        return UiAssignmentValue("expression", text=text)

    @staticmethod
    def object(item: "UiItem") -> "UiAssignmentValue":
        return UiAssignmentValue("object", item=item)

    @staticmethod
    def list(items: list["UiAssignmentValue"]) -> "UiAssignmentValue":
        return UiAssignmentValue("list", items=items)

    def to_json(self) -> dict[str, Any]:
        if self.kind == "expression":
            return {"kind": "expression", "text": self.text or ""}
        if self.kind == "object":
            assert self.item is not None
            return {"kind": "object", "item": self.item.to_json()}
        if self.kind == "list":
            return {"kind": "list", "items": [i.to_json() for i in self.items or []]}
        raise QtError(f"unknown assignment value kind: {self.kind}")


@dataclass
class UiAssignment:
    target: str
    value: UiAssignmentValue

    def to_json(self) -> dict[str, Any]:
        return {"target": self.target, "value": self.value.to_json()}


@dataclass
class UiItem:
    type_name: str
    id: str | None = None
    properties: list[UiProperty] = field(default_factory=list)
    assignments: list[UiAssignment] = field(default_factory=list)
    signals: list[UiSignal] = field(default_factory=list)
    handlers: list[UiHandler] = field(default_factory=list)
    children: list["UiItem"] = field(default_factory=list)

    def to_json(self) -> dict[str, Any]:
        out: dict[str, Any] = {"type_name": self.type_name}
        if self.id is not None:
            out["id"] = self.id
        out["properties"] = [p.to_json() for p in self.properties]
        out["assignments"] = [a.to_json() for a in self.assignments]
        out["signals"] = [s.to_json() for s in self.signals]
        out["handlers"] = [h.to_json() for h in self.handlers]
        out["children"] = [c.to_json() for c in self.children]
        return out


@dataclass
class UiModule:
    source: str
    imports: list[UiImport]
    pragmas: list[str]
    root: UiItem
    state_machine: dict[str, Any] | None = None

    def to_json(self) -> dict[str, Any]:
        out: dict[str, Any] = {
            "version": QT_IR_VERSION,
            "source": self.source,
            "imports": [i.to_json() for i in self.imports],
            "pragmas": self.pragmas,
            "root": self.root.to_json(),
        }
        if self.state_machine is not None:
            out["state_machine"] = self.state_machine
        return out


def strip_comments(src: str) -> str:
    out: list[str] = []
    i = 0
    in_str: str | None = None
    while i < len(src):
        c = src[i]
        if in_str:
            out.append(c)
            if c == "\\" and i + 1 < len(src):
                out.append(src[i + 1])
                i += 2
                continue
            if c == in_str:
                in_str = None
            i += 1
            continue
        if c in ("'", '"'):
            in_str = c
            out.append(c)
            i += 1
            continue
        if src.startswith("//", i):
            while i < len(src) and src[i] not in "\r\n":
                i += 1
            continue
        if src.startswith("/*", i):
            end = src.find("*/", i + 2)
            if end == -1:
                raise QtError("unterminated block comment")
            out.append("\n" * src[i:end + 2].count("\n"))
            i = end + 2
            continue
        out.append(c)
        i += 1
    return "".join(out)


def is_ident_start(c: str) -> bool:
    return c.isalpha() or c == "_"


def is_ident_cont(c: str) -> bool:
    return c.isalnum() or c == "_"


class QmlParser:
    """Small recursive-descent parser for rlvgl's structural QML subset."""

    def __init__(self, source: str, path: Path):
        self.src = strip_comments(source)
        self.path = path
        self.pos = 0
        self.imports: list[UiImport] = []
        self.pragmas: list[str] = []

    def parse(self) -> UiModule:
        while True:
            self.skip_ws()
            if self.eof():
                raise QtError("expected QML root object")
            if self.peek_word("import"):
                self.imports.append(self.parse_import())
                continue
            if self.peek_word("pragma"):
                self.pragmas.append(self.read_line().strip())
                continue
            root = self.parse_item()
            self.skip_ws()
            if not self.eof():
                raise QtError(f"unexpected trailing content at byte {self.pos}")
            module = UiModule(str(self.path), self.imports, self.pragmas, root)
            attach_scjson_side_file(module, self.path)
            return module

    def eof(self) -> bool:
        return self.pos >= len(self.src)

    def skip_ws(self) -> None:
        while not self.eof() and self.src[self.pos].isspace():
            self.pos += 1

    def peek_word(self, word: str) -> bool:
        self.skip_ws()
        end = self.pos + len(word)
        if self.src[self.pos:end] != word:
            return False
        if end < len(self.src) and is_ident_cont(self.src[end]):
            return False
        return True

    def expect(self, lit: str) -> None:
        self.skip_ws()
        if not self.src.startswith(lit, self.pos):
            saw = self.src[self.pos:self.pos + 24].replace("\n", "\\n")
            raise QtError(f"expected `{lit}` at byte {self.pos}, saw `{saw}`")
        self.pos += len(lit)

    def read_line(self) -> str:
        start = self.pos
        while not self.eof() and self.src[self.pos] not in "\r\n":
            self.pos += 1
        return self.src[start:self.pos]

    def read_ident(self, dotted: bool = False) -> str:
        self.skip_ws()
        start = self.pos
        if self.eof() or not is_ident_start(self.src[self.pos]):
            raise QtError(f"expected identifier at byte {self.pos}")
        self.pos += 1
        while not self.eof():
            c = self.src[self.pos]
            if is_ident_cont(c) or (dotted and c == "."):
                self.pos += 1
            else:
                break
        return self.src[start:self.pos]

    def parse_import(self) -> UiImport:
        self.expect("import")
        self.skip_ws()
        if not self.eof() and self.src[self.pos] in ("'", '"'):
            module = self.read_string_token()
        else:
            module = self.read_ident(dotted=True)
        version: str | None = None
        alias: str | None = None
        self.skip_ws()
        if not self.eof() and self.src[self.pos].isdigit():
            version = self.read_bare_token()
        self.skip_ws()
        if self.peek_word("as"):
            self.expect("as")
            alias = self.read_ident(dotted=False)
        self.consume_statement_end()
        return UiImport(module, version, alias)

    def read_bare_token(self) -> str:
        self.skip_ws()
        start = self.pos
        while not self.eof() and not self.src[self.pos].isspace() and self.src[self.pos] not in "{}[]();:":
            self.pos += 1
        return self.src[start:self.pos]

    def read_string_token(self) -> str:
        self.skip_ws()
        quote = self.src[self.pos]
        start = self.pos
        self.pos += 1
        while not self.eof():
            c = self.src[self.pos]
            self.pos += 1
            if c == "\\" and not self.eof():
                self.pos += 1
                continue
            if c == quote:
                return self.src[start + 1:self.pos - 1]
        raise QtError("unterminated string literal")

    def consume_statement_end(self) -> None:
        while not self.eof() and self.src[self.pos] in " \t":
            self.pos += 1
        if not self.eof() and self.src[self.pos] == ";":
            self.pos += 1
        while not self.eof() and self.src[self.pos] in "\r\n":
            self.pos += 1

    def parse_item(self, forced_type: str | None = None) -> UiItem:
        type_name = forced_type if forced_type is not None else self.read_ident(dotted=True)
        self.expect("{")
        item = UiItem(type_name=type_name)
        while True:
            self.skip_ws()
            if self.eof():
                raise QtError(f"unterminated `{type_name}` block")
            if self.src[self.pos] == "}":
                self.pos += 1
                return item
            if self.peek_word("readonly") or self.peek_word("default") or self.peek_word("property"):
                item.properties.append(self.parse_property())
                continue
            if self.peek_word("signal"):
                item.signals.append(self.parse_signal())
                continue
            if self.peek_word("enum"):
                self.skip_enum()
                continue
            if self.peek_word("function"):
                self.skip_function()
                continue
            name = self.read_ident(dotted=True)
            self.skip_ws()
            if not self.eof() and self.src[self.pos] == "{":
                nested = self.parse_item(forced_type=name)
                if name[:1].isupper() or "." in name:
                    item.children.append(nested)
                else:
                    item.assignments.append(UiAssignment(name, UiAssignmentValue.object(nested)))
                self.consume_statement_end()
                continue
            if not self.eof() and self.src[self.pos] == ":":
                self.pos += 1
                if name == "id":
                    item.id = self.read_expr_until_statement().strip()
                    self.consume_statement_end()
                    continue
                if name.startswith("on") and len(name) > 2 and name[2].isupper():
                    item.handlers.append(UiHandler(name, self.read_handler_body()))
                    self.consume_statement_end()
                    continue
                item.assignments.append(UiAssignment(name, self.parse_assignment_value()))
                self.consume_statement_end()
                continue
            raise QtError(f"expected `:` or `{{` after `{name}` at byte {self.pos}")

    def parse_property(self) -> UiProperty:
        readonly = False
        default_kw = False
        if self.peek_word("default"):
            self.expect("default")
            default_kw = True
        if self.peek_word("readonly"):
            self.expect("readonly")
            readonly = True
        self.expect("property")
        ty = self.read_ident(dotted=True)
        name = self.read_ident(dotted=False)
        self.skip_ws()
        default_value = None
        if not self.eof() and self.src[self.pos] == ":":
            self.pos += 1
            default_value = self.read_expr_until_statement().strip()
        self.consume_statement_end()
        return UiProperty(name, ty, default_value, readonly, default_kw)

    def parse_signal(self) -> UiSignal:
        self.expect("signal")
        name = self.read_ident(dotted=False)
        params: list[UiSignalParam] = []
        self.expect("(")
        while True:
            self.skip_ws()
            if not self.eof() and self.src[self.pos] == ")":
                self.pos += 1
                break
            ty = self.read_ident(dotted=True)
            pname = self.read_ident(dotted=False)
            params.append(UiSignalParam(pname, ty))
            self.skip_ws()
            if not self.eof() and self.src[self.pos] == ",":
                self.pos += 1
                continue
            self.expect(")")
            break
        self.consume_statement_end()
        return UiSignal(name, params)

    def skip_enum(self) -> None:
        self.expect("enum")
        self.read_ident(dotted=False)
        self.skip_ws()
        if not self.eof() and self.src[self.pos] == "{":
            self.read_balanced("{", "}")
        self.consume_statement_end()

    def skip_function(self) -> None:
        self.expect("function")
        self.read_ident(dotted=False)
        self.skip_ws()
        if not self.eof() and self.src[self.pos] == "(":
            self.read_balanced("(", ")")
        self.skip_ws()
        if not self.eof() and self.src[self.pos] == "{":
            self.read_balanced("{", "}")
        self.consume_statement_end()

    def parse_assignment_value(self) -> UiAssignmentValue:
        self.skip_ws()
        if not self.eof() and self.src[self.pos] == "[":
            return UiAssignmentValue.list(self.parse_value_list())
        mark = self.pos
        if not self.eof() and is_ident_start(self.src[self.pos]):
            type_name = self.read_ident(dotted=True)
            self.skip_ws()
            if not self.eof() and self.src[self.pos] == "{":
                return UiAssignmentValue.object(self.parse_item(forced_type=type_name))
            self.pos = mark
        return UiAssignmentValue.expression(self.read_expr_until_statement().strip())

    def parse_value_list(self) -> list[UiAssignmentValue]:
        self.expect("[")
        items: list[UiAssignmentValue] = []
        while True:
            self.skip_ws()
            if self.eof():
                raise QtError("unterminated list assignment")
            if self.src[self.pos] == "]":
                self.pos += 1
                return items
            items.append(self.parse_assignment_value())
            self.skip_ws()
            if not self.eof() and self.src[self.pos] in ",;":
                self.pos += 1

    def read_handler_body(self) -> str:
        self.skip_ws()
        if not self.eof() and self.src[self.pos] == "{":
            body = self.read_balanced("{", "}")
            return body[1:-1].strip()
        return self.read_expr_until_statement().strip()

    def read_balanced(self, open_ch: str, close_ch: str) -> str:
        self.skip_ws()
        if self.eof() or self.src[self.pos] != open_ch:
            raise QtError(f"expected `{open_ch}` at byte {self.pos}")
        start = self.pos
        depth = 0
        in_str: str | None = None
        while not self.eof():
            c = self.src[self.pos]
            if in_str:
                self.pos += 1
                if c == "\\" and not self.eof():
                    self.pos += 1
                elif c == in_str:
                    in_str = None
                continue
            if c in ("'", '"'):
                in_str = c
                self.pos += 1
                continue
            if c == open_ch:
                depth += 1
            elif c == close_ch:
                depth -= 1
                if depth == 0:
                    self.pos += 1
                    return self.src[start:self.pos]
            self.pos += 1
        raise QtError(f"unterminated balanced `{open_ch}{close_ch}` block")

    def read_expr_until_statement(self) -> str:
        self.skip_ws()
        start = self.pos
        depth = 0
        in_str: str | None = None
        while not self.eof():
            c = self.src[self.pos]
            if in_str:
                self.pos += 1
                if c == "\\" and not self.eof():
                    self.pos += 1
                elif c == in_str:
                    in_str = None
                continue
            if c in ("'", '"'):
                in_str = c
                self.pos += 1
                continue
            if c in "([{":
                depth += 1
            elif c in ")]}":
                if depth == 0:
                    break
                depth -= 1
            if depth == 0 and c in ";\r\n":
                break
            self.pos += 1
        return self.src[start:self.pos]


def parse_module(path: Path) -> UiModule:
    return QmlParser(path.read_text(), path).parse()


def qml_files(path: Path) -> list[Path]:
    return sorted(p for p in path.iterdir() if p.suffix == ".qml")


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)


def attach_scjson_side_file(module: UiModule, qml_path: Path) -> None:
    side = qml_path.with_suffix(".scjson")
    if not side.exists():
        return
    data = json.loads(side.read_text())
    module.state_machine = scjson_to_state_machine(data, side)


def scjson_to_state_machine(data: dict[str, Any], path: Path) -> dict[str, Any]:
    stem = path.stem.replace("-", "_")
    states: list[dict[str, Any]] = []
    transitions: list[dict[str, Any]] = []
    scripts: list[dict[str, Any]] = []
    for state in data.get("state", []):
        sid = state.get("id")
        if not sid:
            continue
        states.append({"id": sid})
        for idx, tr in enumerate(state.get("transition", [])):
            target = tr.get("target")
            if isinstance(target, list):
                target_id = target[0] if target else None
            else:
                target_id = target
            transitions.append({
                "source": sid,
                "event": tr.get("event"),
                "target": target_id,
                "cond": tr.get("cond"),
                "actions": [],
            })
            for script in tr.get("script", []):
                name = script.get("name") if isinstance(script, dict) else None
                if name:
                    scripts.append({
                        "name": name,
                        "origin": {"kind": "transition", "index": idx, "from": sid, "to": target_id},
                    })
    datamodel: list[dict[str, Any]] = []
    dm = data.get("datamodel")
    if isinstance(dm, dict):
        for item in dm.get("data", []):
            if isinstance(item, dict) and item.get("id"):
                field: dict[str, Any] = {"id": item["id"]}
                if "expr" in item:
                    try:
                        field["initial"] = float(item["expr"])
                    except (TypeError, ValueError):
                        pass
                datamodel.append(field)
    initial = data.get("initial")
    if isinstance(initial, list):
        initial = initial[0] if initial else None
    return {
        "id": stem,
        "source": path.name,
        "initial": initial,
        "states": states,
        "transitions": transitions,
        "datamodel": datamodel,
        "scripts": scripts,
    }


def parse_string_literal(expr: str | None) -> str | None:
    if expr is None:
        return None
    s = expr.strip()
    if len(s) >= 2 and s[0] == s[-1] and s[0] in ("'", '"'):
        return bytes(s[1:-1], "utf-8").decode("unicode_escape")
    return None


def lookup_assignment(item: UiItem, target: str) -> str | None:
    for assignment in item.assignments:
        if assignment.target == target and assignment.value.kind == "expression":
            return assignment.value.text or ""
    return None


def iter_object_items(value: UiAssignmentValue, expected: str) -> Iterable[UiItem]:
    if value.kind == "object" and value.item and value.item.type_name == expected:
        yield value.item
    elif value.kind == "list":
        for child in value.items or []:
            if child.kind == "object" and child.item and child.item.type_name == expected:
                yield child.item


def command_ingest(args: argparse.Namespace) -> None:
    input_path = Path(args.input)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    if input_path.is_dir():
        for qml in qml_files(input_path):
            module = parse_module(qml)
            write_text(out_dir / f"{qml.stem}.qt-ir.json", json.dumps(module.to_json(), indent=2) + "\n")
    else:
        module = parse_module(input_path)
        write_text(out_dir / "qt-ir.json", json.dumps(module.to_json(), indent=2) + "\n")


def command_check(args: argparse.Namespace) -> None:
    parse_module(Path(args.input))


def schema_document() -> dict[str, Any]:
    return {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$id": QT_IR_SCHEMA_ID,
        "$comment": QT_IR_SCHEMA_COMMENT,
        "title": "UiModule",
        "type": "object",
        "required": ["version", "source", "imports", "pragmas", "root"],
        "properties": {
            "version": {"type": "integer", "const": QT_IR_VERSION},
            "source": {"type": "string"},
            "imports": {"type": "array"},
            "pragmas": {"type": "array", "items": {"type": "string"}},
            "root": {"type": "object"},
            "state_machine": {"type": "object"},
        },
    }


def command_schema(args: argparse.Namespace) -> None:
    text = json.dumps(schema_document(), indent=2) + "\n"
    if args.out:
        write_text(Path(args.out), text)
    else:
        print(text, end="")


def rust_str(s: str) -> str:
    return json.dumps(s)


def render_data_rs(module: UiModule) -> str:
    out: list[str] = [
        "// SPDX-License-Identifier: MIT",
        "//",
        f"// Generated by `rlvgl-creator qt emit` from `{module.source}`.",
        "// Python utility mirror: scripts/lvglpp_qt.py.",
        "",
        "#![allow(dead_code)]",
        "",
        f"pub const QT_EMIT_VERSION: u32 = {QT_EMIT_VERSION_DATA};",
        f"pub const QT_IR_VERSION: u32 = {QT_IR_VERSION};",
        f"pub const QT_SOURCE: &str = {rust_str(module.source)};",
        "",
        "#[derive(Debug, Clone, Copy)]",
        "pub struct Node {",
        "    pub type_name: &'static str,",
        "    pub id: Option<&'static str>,",
        "    pub assignments: &'static [Assignment],",
        "    pub children: &'static [Node],",
        "}",
        "",
        "#[derive(Debug, Clone, Copy)]",
        "pub struct Assignment {",
        "    pub target: &'static str,",
        "    pub value: &'static str,",
        "}",
        "",
        "#[rustfmt::skip]",
        f"pub static SCREEN: Node = {render_node(module.root, 0)};",
        "",
    ]
    return "\n".join(out)


def render_node(item: UiItem, indent: int) -> str:
    pad = " " * indent
    pad4 = " " * (indent + 4)
    lines = ["Node {"]
    lines.append(f"{pad4}type_name: {rust_str(item.type_name)},")
    lines.append(f"{pad4}id: {('Some(' + rust_str(item.id) + ')') if item.id else 'None'},")
    for prop_count, label in ((len(item.properties), "property declaration"), (len(item.signals), "signal declaration"), (len(item.handlers), "signal handler")):
        if prop_count:
            suffix = "s" if prop_count != 1 else ""
            lines.append(f"{pad4}// emitter-skipped (python): {prop_count} {label}{suffix}")
    lines.append(f"{pad4}assignments: &[")
    for assignment in item.assignments:
        if assignment.value.kind == "expression":
            lines.append(f"{pad4}    Assignment {{")
            lines.append(f"{pad4}        target: {rust_str(assignment.target)},")
            lines.append(f"{pad4}        value: {rust_str(assignment.value.text or '')},")
            lines.append(f"{pad4}    }},")
        else:
            lines.append(f"{pad4}    // emitter-skipped (python): {assignment.target}: <{assignment.value.kind}>")
    lines.append(f"{pad4}],")
    lines.append(f"{pad4}children: &[")
    for child in item.children:
        child_text = render_node(child, indent + 8)
        lines.append(" " * (indent + 8) + child_text.replace("\n", "\n" + " " * (indent + 8)) + ",")
    lines.append(f"{pad4}],")
    lines.append(f"{pad}}}")
    return "\n".join(lines)


def render_rlvgl_rs(module: UiModule) -> str:
    return "\n".join([
        "// SPDX-License-Identifier: MIT",
        "//",
        f"// Generated by scripts/lvglpp_qt.py from `{module.source}`.",
        "// This is a conservative Python fallback. Use Rust rlvgl-creator for",
        "// canonical widget-specific QT-03b/QT-05 lowering.",
        "",
        "#![allow(dead_code)]",
        f"pub const QT_EMIT_VERSION_RLVGL: u32 = {QT_EMIT_VERSION_RLVGL};",
        f"pub const QT_IR_VERSION: u32 = {QT_IR_VERSION};",
        f"pub const QT_SOURCE: &str = {rust_str(module.source)};",
        "",
        "pub fn qt_source_tree_json() -> &'static str {",
        f"    r#\"{json.dumps(module.to_json(), indent=2)}\"#",
        "}",
        "",
    ])


def command_emit(args: argparse.Namespace) -> None:
    input_path = Path(args.input)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    files = qml_files(input_path) if input_path.is_dir() else [input_path]
    for qml in files:
        module = parse_module(qml)
        if args.target == "data":
            write_text(out_dir / f"{qml.stem}.rs", render_data_rs(module))
        else:
            write_text(out_dir / f"{qml.stem}.rlvgl.rs", render_rlvgl_rs(module))


def walk_qml_state_machine(item: UiItem, source: str) -> dict[str, Any] | None:
    states_assignment = next((a for a in item.assignments if a.target == "states"), None)
    transitions_assignment = next((a for a in item.assignments if a.target == "transitions"), None)
    if states_assignment is None and transitions_assignment is None:
        return None
    states: list[dict[str, Any]] = []
    initial: list[str] = []
    if states_assignment:
        for state_item in iter_object_items(states_assignment.value, "State"):
            name = parse_string_literal(lookup_assignment(state_item, "name"))
            if not name:
                raise QtError(f"State block in {source} has no literal-string name")
            if (lookup_assignment(state_item, "initial") or "").strip() == "true":
                if initial:
                    raise QtError(f"multiple initial states in {source}")
                initial.append(name)
            states.append({"id": name, "transition": []})
    if transitions_assignment:
        for transition in iter_object_items(transitions_assignment.value, "Transition"):
            src = parse_string_literal(lookup_assignment(transition, "from"))
            dst = parse_string_literal(lookup_assignment(transition, "to"))
            event = parse_string_literal(lookup_assignment(transition, "event"))
            if not src or not dst:
                raise QtError(f"Transition block in {source} needs literal from/to")
            host = next((s for s in states if s["id"] == src), None)
            if host is None:
                raise QtError(f"Transition references unknown source state `{src}`")
            tr: dict[str, Any] = {}
            if event:
                tr["event"] = event
            tr["target"] = [dst]
            host["transition"].append(tr)
    return {
        "state": states,
        "initial": initial,
        "datamodel_attribute": "null",
        "other_attributes": {"_comment": f"QT-05d emit-scjson: {source}"},
    }


def resolve_out(input_path: Path, out: str | None, suffix: str) -> Path:
    if out:
        p = Path(out)
        if p.exists() and p.is_dir():
            return p / f"{input_path.stem}{suffix}"
        if not p.suffix:
            return p / f"{input_path.stem}{suffix}"
        return p
    return input_path.with_name(f"{input_path.stem}{suffix}")


def command_emit_scjson(args: argparse.Namespace) -> None:
    input_path = Path(args.input)
    if input_path.is_dir():
        out_dir = Path(args.out) if args.out else input_path
        out_dir.mkdir(parents=True, exist_ok=True)
        files = qml_files(input_path)
        pairs = [(qml, out_dir / f"{qml.stem}.scjson") for qml in files]
    else:
        pairs = [(input_path, resolve_out(input_path, args.out, ".scjson"))]
    for qml, out_path in pairs:
        sm = walk_qml_state_machine(parse_module(qml).root, str(qml))
        if sm is not None:
            write_text(out_path, json.dumps(sm, indent=2) + "\n")


def render_externals(sm: dict[str, Any], qml_stem: str, qml_source: str) -> str:
    sm_id = sm.get("id", qml_stem)
    lines = [
        "// SPDX-License-Identifier: MIT",
        "//",
        f"// Generated by `rlvgl-creator qt emit-externals` from `{qml_source}`.",
        "",
        "#![allow(dead_code)]",
        "#![allow(unused_variables)]",
        "",
        f"use {sm_id}_gen::{{Externals, Machine}};",
        "",
        f"pub const QT_EXTERNALS_VERSION: u32 = {QT_EXTERNALS_VERSION};",
        "",
        "pub struct ScreenExternals;",
        "",
        "impl ScreenExternals { pub fn new() -> Self { Self } }",
        "impl Default for ScreenExternals { fn default() -> Self { Self::new() } }",
        "",
        "impl Externals for ScreenExternals {",
    ]
    for script in sm.get("scripts", []):
        name = script.get("name")
        if not name:
            continue
        lines.extend([
            f"    fn {name}(&mut self, m: &mut Machine) {{",
            f"        // QT-05e externals-stub: {name}",
            "        let _ = m;",
            "    }",
            "",
        ])
    lines.append("}")
    return "\n".join(lines).rstrip() + "\n"


def command_emit_externals(args: argparse.Namespace) -> None:
    input_path = Path(args.input)
    if input_path.is_dir():
        out_dir = Path(args.out) if args.out else input_path
        out_dir.mkdir(parents=True, exist_ok=True)
        pairs = [(qml, out_dir / f"{qml.stem}_externals.rs") for qml in qml_files(input_path)]
    else:
        pairs = [(input_path, resolve_out(input_path, args.out, "_externals.rs"))]
    for qml, out_path in pairs:
        module = parse_module(qml)
        if module.state_machine and module.state_machine.get("scripts"):
            write_text(out_path, render_externals(module.state_machine, qml.stem, str(qml)))


def render_tokens_yaml(item: UiItem, source: str) -> str | None:
    colors: dict[str, str] = {}
    dark: dict[str, str] = {}
    spacing: dict[str, int] = {}
    radii: dict[str, int] = {}
    fonts: dict[str, str] = {}
    for prop in item.properties:
        default = prop.default_value
        if default is None:
            continue
        if prop.ty == "color":
            value = parse_string_literal(default)
            if value and re.fullmatch(r"#[0-9A-Fa-f]{3,8}", value):
                if prop.name.endswith("_dark"):
                    dark[prop.name[:-5]] = value
                else:
                    colors[prop.name] = value
        elif prop.ty == "int":
            try:
                value_i = int(default.strip())
            except ValueError:
                continue
            if prop.name.startswith("spacing_"):
                spacing[prop.name[8:]] = value_i
            elif prop.name.startswith("radius_"):
                radii[prop.name[7:]] = value_i
        elif prop.ty == "string":
            value_s = parse_string_literal(default)
            if value_s and prop.name.startswith("font_"):
                fonts[prop.name[5:]] = value_s
    if not any((colors, dark, spacing, radii, fonts)):
        return None
    lines = [
        "# Auto-generated from Qt theme by rlvgl-creator (QT-06)",
        f"# QT-06 theme: {source}",
        "version: 1",
        "colors:",
    ]
    lines += [f"  {k}: \"{colors[k]}\"" for k in sorted(colors)]
    lines.append("spacing:")
    lines += [f"  {k}: {spacing[k]}" for k in sorted(spacing)]
    lines.append("radii:")
    lines += [f"  {k}: {radii[k]}" for k in sorted(radii)]
    lines.append("fonts:")
    lines += [f"  {k}: \"{fonts[k]}\"" for k in sorted(fonts)]
    if dark:
        lines += ["modes:", "  dark:", "    colors:"]
        lines += [f"      {k}: \"{dark[k]}\"" for k in sorted(dark)]
    return "\n".join(lines) + "\n"


def command_emit_tokens(args: argparse.Namespace) -> None:
    emit_yaml_family(args, ".tokens.yaml", lambda qml: render_tokens_yaml(parse_module(qml).root, str(qml)))


def strip_qrc(path: str) -> str:
    if path.startswith("qrc:///"):
        return path[7:]
    if path.startswith("qrc:/"):
        return path[5:]
    return path


def is_image_path(path: str) -> bool:
    return path.lower().endswith((".png", ".jpg", ".jpeg", ".gif", ".svg", ".bmp", ".webp"))


def extract_asset_literals(expr: str) -> list[str]:
    out = []
    for match in re.finditer(r"""(['"])(.*?)(?<!\\)\1""", expr):
        lit = match.group(2)
        if lit.lower().startswith("qrc:") or is_image_path(lit):
            out.append(lit)
    return out


def collect_assets(item: UiItem, images: set[str], fonts: set[str]) -> None:
    stripped = item.type_name.rsplit(".", 1)[-1]
    if stripped in {"Image", "BorderImage", "AnimatedImage"}:
        raw = lookup_assignment(item, "source")
        literal = parse_string_literal(raw)
        if literal:
            images.add(strip_qrc(literal))
        elif raw:
            for lit in extract_asset_literals(raw):
                images.add(strip_qrc(lit))
    raw_font = lookup_assignment(item, "font.family")
    literal_font = parse_string_literal(raw_font)
    if literal_font:
        fonts.add(literal_font)
    if stripped == "Font":
        family = parse_string_literal(lookup_assignment(item, "family"))
        if family:
            fonts.add(family)
    for assignment in item.assignments:
        if assignment.target == "font" and assignment.value.kind == "object" and assignment.value.item:
            family = parse_string_literal(lookup_assignment(assignment.value.item, "family"))
            if family:
                fonts.add(family)
    for child in item.children:
        collect_assets(child, images, fonts)


def quote_yaml(s: str) -> str:
    if not s or any(c in s for c in " \t:#\"'[]{},"):
        return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'
    return s


def render_assets_yaml(module: UiModule) -> str | None:
    images: set[str] = set()
    fonts: set[str] = set()
    collect_assets(module.root, images, fonts)
    if not images and not fonts:
        return None
    lines = [
        "# Auto-generated from Qt project by rlvgl-creator (QT-07)",
        f"# QT-07 assets: {module.source}",
        "version: 1",
        "images:",
    ]
    lines += [f"  - {quote_yaml(p)}" for p in sorted(images)]
    lines.append("fonts:")
    lines += [f"  - {quote_yaml(f)}" for f in sorted(fonts)]
    return "\n".join(lines) + "\n"


def command_list_assets(args: argparse.Namespace) -> None:
    emit_yaml_family(args, ".assets.yaml", lambda qml: render_assets_yaml(parse_module(qml)))


def emit_yaml_family(args: argparse.Namespace, suffix: str, render: Any) -> None:
    input_path = Path(args.input)
    if input_path.is_dir():
        out_dir = Path(args.out) if args.out else input_path
        out_dir.mkdir(parents=True, exist_ok=True)
        pairs = [(qml, out_dir / f"{qml.stem}{suffix}") for qml in qml_files(input_path)]
    else:
        pairs = [(input_path, resolve_out(input_path, args.out, suffix))]
    for qml, out_path in pairs:
        text = render(qml)
        if text:
            write_text(out_path, text)


def parse_qmldir(content: str) -> dict[str, Any]:
    manifest: dict[str, Any] = {
        "module": None,
        "types": [],
        "singletons": [],
        "internals": [],
        "imports": [],
        "depends": [],
        "plugins": [],
        "other": [],
    }
    for raw in content.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        tokens = line.split()
        if len(tokens) == 2 and tokens[0] == "module":
            manifest["module"] = tokens[1]
        elif len(tokens) == 4 and tokens[0] == "singleton":
            manifest["singletons"].append({"name": tokens[1], "version": tokens[2], "file": tokens[3]})
        elif len(tokens) == 3 and tokens[0] == "internal":
            manifest["internals"].append({"name": tokens[1], "version": None, "file": tokens[2]})
        elif tokens[0] in {"import", "depends"} and len(tokens) in {2, 3}:
            manifest[tokens[0] + "s" if tokens[0] == "import" else "depends"].append({
                "module": tokens[1],
                "version": tokens[2] if len(tokens) == 3 else None,
            })
        elif tokens[0] == "plugin" and len(tokens) in {2, 3}:
            manifest["plugins"].append({"name": tokens[1], "path": tokens[2] if len(tokens) == 3 else None})
        elif len(tokens) == 3 and tokens[2].endswith(".qml"):
            manifest["types"].append({"name": tokens[0], "version": tokens[1], "file": tokens[2]})
        else:
            manifest["other"].append(line)
    return manifest


def render_qmldir_yaml(manifest: dict[str, Any], source: str) -> str:
    lines = [f"# QT-08b qmldir: {source}", "version: 1", f"module: {manifest['module'] if manifest['module'] else 'null'}"]
    for section in ("types", "singletons", "internals"):
        lines.append(f"{section}:")
        for item in manifest[section]:
            version = f"\"{item['version']}\"" if item["version"] is not None else "null"
            lines.append(f"  - {{ name: {item['name']}, version: {version}, file: {item['file']} }}")
    for section in ("imports", "depends"):
        lines.append(f"{section}:")
        for item in manifest[section]:
            version = f"\"{item['version']}\"" if item["version"] is not None else "null"
            lines.append(f"  - {{ module: {item['module']}, version: {version} }}")
    lines.append("plugins:")
    for item in manifest["plugins"]:
        path = f"\"{item['path']}\"" if item["path"] is not None else "null"
        lines.append(f"  - {{ name: {item['name']}, path: {path} }}")
    lines.append("other:")
    lines += [f"  - {quote_yaml(s)}" for s in manifest["other"]]
    return "\n".join(lines) + "\n"


def command_list_qmldir(args: argparse.Namespace) -> None:
    input_path = Path(args.input)
    qmldir = input_path / "qmldir" if input_path.is_dir() else input_path
    if not qmldir.exists():
        raise QtError(f"expected qmldir file at {qmldir}")
    dirname = input_path.name if input_path.is_dir() else (input_path.parent.name or "untitled")
    out = Path(args.out) if args.out else qmldir.parent / f"{dirname}.qmldir.yaml"
    if out.exists() and out.is_dir():
        out = out / f"{dirname}.qmldir.yaml"
    text = render_qmldir_yaml(parse_qmldir(qmldir.read_text()), str(qmldir))
    write_text(out, text)


def render_qrc_yaml(path: Path) -> str:
    root = ET.fromstring(path.read_text())
    if root.tag != "RCC":
        raise QtError("expected <RCC> root element")
    lines = [f"# QT-08c qrc: {path}", "version: 1"]
    version = root.attrib.get("version")
    lines.append(f"rcc_version: \"{version}\"" if version else "rcc_version: null")
    lines.append("resources:")
    for res in root:
        if res.tag != "qresource":
            raise QtError("only <qresource> is allowed under <RCC>")
        prefix = res.attrib.get("prefix")
        lang = res.attrib.get("lang")
        prefix_value = f"\"{prefix}\"" if prefix else "null"
        lang_value = f"\"{lang}\"" if lang else "null"
        lines.append(f"  - prefix: {prefix_value}")
        lines.append(f"    lang: {lang_value}")
        lines.append("    files:")
        for file_node in res:
            if file_node.tag != "file":
                raise QtError("only <file> is allowed under <qresource>")
            alias = file_node.attrib.get("alias")
            lines.append(f"      - {{ path: {(file_node.text or '').strip()}, alias: {alias if alias else 'null'} }}")
    return "\n".join(lines) + "\n"


def command_list_qrc(args: argparse.Namespace) -> None:
    input_path = Path(args.input)
    if input_path.is_dir():
        out_dir = Path(args.out) if args.out else input_path
        out_dir.mkdir(parents=True, exist_ok=True)
        pairs = [(qrc, out_dir / f"{qrc.stem}.qrc.yaml") for qrc in sorted(input_path.glob("*.qrc"))]
    else:
        pairs = [(input_path, resolve_out(input_path, args.out, ".qrc.yaml"))]
    for qrc, out_path in pairs:
        write_text(out_path, render_qrc_yaml(qrc))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Python lvglpp Qt/QML utility")
    sub = parser.add_subparsers(dest="cmd", required=True)

    ingest = sub.add_parser("ingest")
    ingest.add_argument("input")
    ingest.add_argument("out")
    ingest.set_defaults(func=command_ingest)

    check = sub.add_parser("check")
    check.add_argument("input")
    check.set_defaults(func=command_check)

    schema = sub.add_parser("schema")
    schema.add_argument("--out")
    schema.set_defaults(func=command_schema)

    emit = sub.add_parser("emit")
    emit.add_argument("--target", choices=("data", "rlvgl"), default="rlvgl")
    emit.add_argument("--scxml-context", help="Accepted for CLI parity; currently unused")
    emit.add_argument("input")
    emit.add_argument("out")
    emit.set_defaults(func=command_emit)

    emit_scjson = sub.add_parser("emit-scjson")
    emit_scjson.add_argument("input")
    emit_scjson.add_argument("out", nargs="?")
    emit_scjson.set_defaults(func=command_emit_scjson)

    emit_externals = sub.add_parser("emit-externals")
    emit_externals.add_argument("input")
    emit_externals.add_argument("out", nargs="?")
    emit_externals.set_defaults(func=command_emit_externals)

    emit_tokens = sub.add_parser("emit-tokens")
    emit_tokens.add_argument("input")
    emit_tokens.add_argument("out", nargs="?")
    emit_tokens.set_defaults(func=command_emit_tokens)

    list_assets = sub.add_parser("list-assets")
    list_assets.add_argument("input")
    list_assets.add_argument("out", nargs="?")
    list_assets.set_defaults(func=command_list_assets)

    list_qmldir = sub.add_parser("list-qmldir")
    list_qmldir.add_argument("input")
    list_qmldir.add_argument("out", nargs="?")
    list_qmldir.set_defaults(func=command_list_qmldir)

    list_qrc = sub.add_parser("list-qrc")
    list_qrc.add_argument("input")
    list_qrc.add_argument("out", nargs="?")
    list_qrc.set_defaults(func=command_list_qrc)

    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        args.func(args)
    except (OSError, json.JSONDecodeError, ET.ParseError, QtError) as exc:
        print(f"lvglpp_qt.py: error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
