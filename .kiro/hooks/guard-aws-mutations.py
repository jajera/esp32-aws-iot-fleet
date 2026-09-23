#!/usr/bin/env python3
"""Block AWS-mutating shell commands unless the operator opts in.

Wired as a Kiro PreToolUse hook. Exit code 2 blocks the tool call
(https://kiro.dev/docs/hooks/).

    export FLEET_ALLOW_AWS=1
"""

from __future__ import annotations

import json
import os
import re
import sys

ALLOW_ENV = "FLEET_ALLOW_AWS"
MAX_COMMAND_CHARS = 2000

READ_ONLY_VERBS = frozenset(
    {
        "describe",
        "list",
        "get",
        "head",
        "search",
        "lookup",
        "select",
        "query",
        "check",
        "estimate",
        "simulate",
        "validate",
        "wait",
        "help",
    }
)

READ_ONLY_S3_SUBCOMMANDS = frozenset({"ls", "presign"})

IAC_PATTERNS = (
    r"\bterraform\s+(apply|destroy|import|taint)\b",
    r"\bcdk\s+(deploy|destroy|import)\b",
    r"\bsam\s+(deploy|sync|delete)\b",
    r"\baws\s+cloudformation\s+(deploy|create-stack|update-stack|delete-stack|execute-change-set)\b",
)

AWS_INVOCATION = re.compile(
    r"(?:^|[;&|]|\$\(|`|\bthen\b|\bdo\b|\bxargs\b)\s*(aws\s+[^\n;&|`)]+)",
    re.IGNORECASE | re.MULTILINE,
)

INTERESTING = ("aws", "terraform", "cdk", "sam")


def iter_strings(node: object):
    if isinstance(node, str):
        yield node
    elif isinstance(node, dict):
        for value in node.values():
            yield from iter_strings(value)
    elif isinstance(node, list):
        for value in node:
            yield from iter_strings(value)


def candidate_commands(raw: str) -> list[str]:
    candidates: list[str] = []
    try:
        payload = json.loads(raw)
    except (json.JSONDecodeError, ValueError):
        payload = None

    if payload is None:
        candidates.append(raw)
    else:
        candidates.extend(iter_strings(payload))

    return [
        text
        for text in candidates
        if 0 < len(text) <= MAX_COMMAND_CHARS
        and any(token in text for token in INTERESTING)
    ]


def is_mutating_aws(command: str) -> bool:
    tokens = command.split()
    if not tokens or tokens[0].lower() != "aws":
        return False

    words = [token for token in tokens[1:] if not token.startswith("-")]
    if len(words) < 2:
        return False

    service, operation = words[0].lower(), words[1].lower()

    if service in {"s3", "s3api"} and operation in READ_ONLY_S3_SUBCOMMANDS:
        return False
    if service == "sts" and operation.startswith("get"):
        return False
    if operation.split("-", 1)[0] in READ_ONLY_VERBS:
        return False
    return True


def main() -> int:
    if os.environ.get(ALLOW_ENV) == "1":
        return 0

    raw = sys.stdin.read()
    if not raw.strip():
        return 0

    for command in candidate_commands(raw):
        for pattern in IAC_PATTERNS:
            if re.search(pattern, command, re.IGNORECASE):
                sys.stderr.write(
                    "Blocked: this repository does not provision infrastructure unprompted.\n"
                    f"  matched: {pattern}\n"
                    f"Author the command in docs/examples instead, or set {ALLOW_ENV}=1.\n"
                    "See .kiro/steering/lab-safety.md.\n"
                )
                return 2

        for match in AWS_INVOCATION.finditer(command):
            invocation = match.group(1).strip()
            if is_mutating_aws(invocation):
                sys.stderr.write(
                    "Blocked: mutating AWS CLI call while editing the fleet hub.\n"
                    f"  {invocation}\n"
                    "Read-only calls (describe / list / get / s3 ls) are allowed.\n"
                    f"To run AWS mutations deliberately, set {ALLOW_ENV}=1.\n"
                    "See .kiro/steering/lab-safety.md.\n"
                )
                return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())
