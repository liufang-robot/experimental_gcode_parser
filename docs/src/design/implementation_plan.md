# Implementation Plan

## Delivery Principles

- small coherent PR slices
- tests + docs in same PR
- keep `./dev/check.sh` green
- merge only after explicit user approval

## Phase Summary

1. Foundation
- machine profile scaffolding
- modal registry scaffolding

2. Syntax Extensions
- comments, M-code syntax baseline, rapid-traverse syntax

3. Modal Integration
- grouped modal behavior rollout

4. Tool/Runtime
- tool-change policies
- runtime-executable M-code subset

## Dependency Order

Current preferred order is maintained in root `IMPLEMENTATION_PLAN.md` Section 4,
including:
- comments before deeper modal/runtime families
- grouped modal rollout before full tool/runtime integration

## Validation Matrix

Per feature slice:
- parser tests
- AIL tests
- executor tests (if runtime behavior changes)
- CLI/stage-output tests when schemas/output change
- docs updates (`SPEC.md`, program/development references)

This is a split summary of root `IMPLEMENTATION_PLAN.md`.
