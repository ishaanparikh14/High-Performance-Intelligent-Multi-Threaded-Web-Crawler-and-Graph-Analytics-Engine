# Interactive Graph Features

## New Node Interaction Features

### 1. Click-to-Highlight Node Connections
**How it works:**
- **Click any node** in the interactive web topology graph to persistently highlight all its connections
- The clicked node turns **green (#10B981)** and becomes the focal point
- **Connected nodes** light up in **cyan (#06B6D4)**
- **Connected links** are brightened and made more visible
- All other nodes and links fade to gray for better contrast

**Visual feedback:**
- Selected node: Larger, green, with enhanced glow
- Connected nodes: Cyan, enlarged, with cyan border
- Unselected nodes: Gray, dimmed
- Active links: Bright cyan, thicker, full opacity
- Inactive links: Gray, thin, low opacity

**Resetting the selection:**
- Click anywhere on the background/empty area to reset and return to normal view
- Or hover over a new node to temporarily highlight different connections

### 2. Tarjan SCC Graphical Representation
**How it works:**
- **SCC Visual Container** shows all discovered communities as interactive circles
- Each circle represents one Strongly Connected Component
- **Circle size** scales with the number of domains in that SCC
- **Color-coded** for easy distinction between different SCCs
- **Hover tooltips** show the member domains

**Visual elements:**
```
┌─────────────────────────┐
│  SCC #0                 │
│       6                 │
│    domains              │
└─────────────────────────┘
   Radial gradient fill
   Glowing border
   Hover tooltip: github.com, npmjs.com, stackoverflow.com, +3
```

**Understanding SCCs:**
- **Size matters**: Larger circles = more domains in mutual link cycles
- **Color diversity**: Different colors help visually separate communities
- **Interconnected**: Domains within one SCC can reach each other through links
- **Communities**: Represents groups of websites that link to each other

## Technical Implementation

### Graph Highlighting (`highlightConnections` function)
- Takes `nodeId`, graph elements, and link data
- `isPersistent` flag distinguishes between hover (temporary) and click (persistent)
- Stores selected state in `window.SELECTED_NODE` for click persistence
- Calculates all connected nodes in O(E) time

### SCC Visualization (`buildSCCVisualization` function)
- Maps each SCC to a visual circle element
- Uses 20-color palette for variety
- Circle radius: `baseRadius = 30 + (size * 8)` pixels
- Applies radial gradient and glow effects
- Renders inline tooltips on hover

## Interaction Tips

1. **Exploring connections**: Click a high-PageRank domain to see what it links to
2. **Finding communities**: Larger SCC circles indicate tight-knit web communities
3. **Chaining exploration**: Click a connected node to explore its network
4. **Reset view**: Click empty space to clear selections

## Performance

- Highlighting is instant (O(V+E) complexity)
- No re-rendering of the entire graph
- Smooth CSS transitions between states
- GPU-accelerated transforms for circles

---

**Example workflow:**
1. Start crawl → Results load
2. Click "Graph Analytics" tab
3. Click any domain node in the interactive topology
4. See all its outgoing links highlighted in cyan
5. Scroll down to "SCC Communities - Visual Representation"
6. Notice size of circles = strength of community connections
