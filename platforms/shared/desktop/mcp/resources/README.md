# Geargrafx MCP Resources

This directory contains documentation resources that are exposed through the Geargrafx MCP server. These resources provide context and reference material to AI assistants for PC Engine / TurboGrafx-16 development and debugging.

## Directory Structure

Resources are organized into categories, each in its own subdirectory:

```
resources/
├── hardware/           # Hardware documentation
│   ├── toc.json       # Table of contents
│   ├── huc6280_cpu.md
│   ├── huc6280_instructions.md
│   ├── huc6280_psg.md
│   ├── huc6270_vdc.md
│   ├── huc6260_vce.md
│   ├── huc6202_vpc.md
│   └── memory_map.md
└── README.md          # This file
```

## How Resources Work

### Resource URIs

Each resource is identified by a URI with the format:

```
geargrafx://<category>/<resource_id>
```

Examples:
- `geargrafx://hardware/huc6280_cpu`
- `geargrafx://hardware/huc6280_instructions`
- `geargrafx://hardware/memory_map`

### Table of Contents (toc.json)

Each category directory must contain a `toc.json` file that lists all resources in that category:

```json
{
  "toc": [
    {
      "uri": "resource_id",
      "title": "Resource Title",
      "description": "Brief description of the resource",
      "mimeType": "text/markdown"
    }
  ]
}
```

**Fields:**
- `uri`: Resource identifier (filename without extension)
- `title`: Human-readable title
- `description`: Brief description for AI assistants
- `mimeType`: Content type (typically `text/markdown` or `text/plain`)

### Resource Files

Resource content files are named `<resource_id>.md` and should be placed in the category directory. They are typically written in Markdown format for readability.

## Adding New Resources

To add a new resource:

1. **Choose or create a category directory** (e.g., `hardware`, `programming`, `audio`, etc.)

2. **Create the resource file** (e.g., `huc6280_timer.md`)

3. **Update the category's toc.json** to include the new resource:

```json
{
  "toc": [
    {
      "uri": "huc6280_timer",
      "title": "HuC6280 Timer",
      "description": "Documentation for the HuC6280's integrated timer",
      "mimeType": "text/markdown"
    }
  ]
}
```

4. **Update `mcp_server.cpp`** to load the new category (if adding a new category):

In the `LoadResources()` method, add:
```cpp
LoadResourcesFromCategory("yourcategory", resourcesPath + "/yourcategory/toc.json");
```

5. **Rebuild the emulator**

## Resource Categories

### Hardware
Technical documentation for PC Engine / TurboGrafx-16 hardware components:
- **HuC6280 CPU** — 8-bit CMOS microprocessor with 65C02 core, MMU, timer, I/O ports
- **HuC6280 Instructions** — Complete instruction set reference with all opcodes and addressing modes
- **HuC6280 PSG** — Programmable Sound Generator: 6 channels, waveform memory, LFO, noise
- **HuC6270 VDC** — Video Display Controller: VRAM, registers, sprites, DMA, SuperGrafx support
- **HuC6260 VCE** — Video Color Encoder: 512-color palette RAM, color table access
- **HuC6202 VPC** — Video Priority Controller (SuperGrafx): dual-VDC priority and windowing
- **Memory Map** — Complete memory system: logical/physical addressing, MMU pages, I/O layout

### Suggested Future Categories

- **audio/** - Sound programming, PSG waveforms, ADPCM techniques
- **video/** - VDC programming, sprite techniques, scrolling effects
- **cdrom/** - CD-ROM programming, sector formats, BIOS calls
- **examples/** - Code snippets and complete example programs
- **tools/** - Development tool documentation
- **bios/** - System BIOS documentation and calls
- **games/** - Game-specific documentation (for reverse engineering)

## Best Practices

### Content Guidelines

1. **Be Accurate**: Resources should contain verified, accurate information
2. **Be Concise**: Focus on essential information; avoid unnecessary verbosity
3. **Use Examples**: Include code examples where appropriate
4. **Reference Sources**: Credit original documentation sources
5. **Keep Updated**: Update resources as new information becomes available

### Markdown Formatting

- Use clear headers (`##`, `###`) for organization
- Include code blocks with syntax highlighting
- Use tables for register/memory layouts
- Add diagrams in ASCII art when helpful
- Keep line length reasonable (~80-100 chars)

### Resource Size

- Keep individual resources focused and manageable
- Split large topics into multiple smaller resources
- Typical size: 100-500 lines of Markdown
- Large resources can be split by sub-topic

## Usage

Resources are automatically loaded when the MCP server starts. AI clients can:

1. **List all resources** via `resources/list` MCP call
2. **Read resource content** via `resources/read` with a URI

AI assistants will automatically fetch relevant resources when they need context about the PC Engine / TurboGrafx-16 hardware or programming.

## Contributing

When adding resources:

1. Follow the existing structure and naming conventions
2. Ensure `toc.json` is valid JSON
3. Use Markdown for text resources
4. Test that resources load correctly after rebuilding
5. Update this README if adding new categories

## License

Resources should follow the same GPL-3.0 license as the Geargrafx emulator, unless they are adaptations of public domain or permissively licensed documentation (in which case, credit the source).
