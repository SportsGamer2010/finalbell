# UI generation

1. Run `ui-spec` for the named screen.
2. Use colors, type, and spacing from `Tools/AI/config/design_system.json`.
3. If `FIGMA_ACCESS_TOKEN` is unset, say the layout is a local specification and Figma was not called.
4. Create a UMG widget only in the editor or in C++ that compiles. Do not claim the widget exists because the JSON exists.
