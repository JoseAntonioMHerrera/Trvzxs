#!/usr/bin/env python3
"""
Generates `humo2_frame_lifecycle.pdf` — a focused, book-style reference on the
per-frame hot path of the humo2 Vulkan renderer.

Content:
  1. Title + TOC
  2. Frame lifecycle overview (the whole drawFrame story, high-level diagram)
  3. Scene data structures touched on the hot path (struct cards + relationships)
  4. drawFrame() source, annotated
  5. Stage-by-stage walkthrough with the actual C code:
       - Transform resolve (preorder SoA walk with dirty-flag skip)
       - Frame globals + view/proj/viewProj
       - Frustum cull (Gribb/Hartmann + Arvo + subtree bulk-skip)
       - Model UBO fill with LOD partitioning
       - Light UBO fill
       - Render graph execute + pass callbacks (depth prepass, model, light)
       - Submit + present

No source appendix. This file is a technical reference, not a code listing.
"""

import os
import re
import textwrap

from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, PageBreak,
    Table, TableStyle, Flowable, KeepTogether
)
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.colors import HexColor
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY
from reportlab.graphics.shapes import Drawing, Rect, String, Line, Polygon
from reportlab.graphics import renderPDF

# ---------------------------------------------------------------------------
# Page + style constants
# ---------------------------------------------------------------------------

PAGE_W, PAGE_H = A4
MARGIN_LEFT = 54
MARGIN_RIGHT = 54
MARGIN_TOP = 60
MARGIN_BOTTOM = 60
CONTENT_W = PAGE_W - MARGIN_LEFT - MARGIN_RIGHT

COLORS = {
    'bg_dark':        HexColor('#1a1a2e'),
    'bg_medium':      HexColor('#16213e'),
    'bg_code':        HexColor('#f4f4f8'),
    'bg_struct':      HexColor('#eef2f7'),
    'bg_struct_alt':  HexColor('#ffffff'),
    'accent':         HexColor('#e94560'),
    'accent2':        HexColor('#0f3460'),
    'text_primary':   HexColor('#1a1a1a'),
    'text_secondary': HexColor('#4a4a6a'),
    'text_code':      HexColor('#2d2d2d'),
    'text_comment':   HexColor('#6a9955'),
    'text_type':      HexColor('#0f766e'),
    'border_light':   HexColor('#d0d0e0'),
    'white':          HexColor('#ffffff'),
}

# ---------------------------------------------------------------------------
# Paragraph styles
# ---------------------------------------------------------------------------

def build_styles():
    styles = getSampleStyleSheet()

    styles.add(ParagraphStyle(
        'DocTitle', fontName='Helvetica-Bold', fontSize=26,
        textColor=COLORS['white'], alignment=TA_CENTER, spaceAfter=12))
    styles.add(ParagraphStyle(
        'DocSubtitle', fontName='Helvetica', fontSize=13,
        textColor=COLORS['white'], alignment=TA_CENTER, spaceAfter=6))
    styles.add(ParagraphStyle(
        'TitleMeta', fontName='Helvetica', fontSize=10,
        textColor=HexColor('#b0b0c0'), alignment=TA_CENTER, spaceAfter=4))
    styles.add(ParagraphStyle(
        'SectionH1', fontName='Helvetica-Bold', fontSize=18,
        textColor=COLORS['bg_dark'], spaceBefore=18, spaceAfter=10))
    styles.add(ParagraphStyle(
        'SectionH2', fontName='Helvetica-Bold', fontSize=13,
        textColor=COLORS['accent2'], spaceBefore=14, spaceAfter=6))
    styles.add(ParagraphStyle(
        'SectionH3', fontName='Helvetica-Bold', fontSize=11,
        textColor=COLORS['text_primary'], spaceBefore=10, spaceAfter=4))
    styles.add(ParagraphStyle(
        'BodyText2', fontName='Helvetica', fontSize=9.5,
        textColor=COLORS['text_primary'], leading=13,
        spaceAfter=6, alignment=TA_JUSTIFY))
    styles.add(ParagraphStyle(
        'Annotation', fontName='Helvetica-Oblique', fontSize=9,
        textColor=COLORS['text_secondary'], leading=12,
        spaceBefore=2, spaceAfter=8, leftIndent=10))
    styles.add(ParagraphStyle(
        'Caption', fontName='Helvetica-Oblique', fontSize=8,
        textColor=COLORS['text_secondary'], alignment=TA_CENTER,
        spaceBefore=4, spaceAfter=12))
    styles.add(ParagraphStyle(
        'TOCLine', fontName='Helvetica', fontSize=10,
        textColor=COLORS['text_primary'], spaceAfter=3, leading=13))
    styles.add(ParagraphStyle(
        'TOCLineBold', fontName='Helvetica-Bold', fontSize=10.5,
        textColor=COLORS['accent2'], spaceBefore=6, spaceAfter=2, leading=13))
    styles.add(ParagraphStyle(
        'BulletText', fontName='Helvetica', fontSize=9.5,
        textColor=COLORS['text_primary'], leading=13,
        spaceAfter=3, leftIndent=14, bulletIndent=4))
    return styles


# ---------------------------------------------------------------------------
# CodeBlock flowable — filename label + line numbers + background + page-split
# ---------------------------------------------------------------------------

class CodeBlock(Flowable):
    def __init__(self, code, filename=None, start_line=1,
                 font_size=7.3, leading=9.6, max_width=None):
        super().__init__()
        self.code = code.rstrip('\n')
        self.filename = filename
        self.start_line = start_line
        self.font_size = font_size
        self.leading = leading
        self.lines = self.code.split('\n')
        self.gutter_w = 28
        self.pad = 5
        self.max_width = max_width or CONTENT_W

        self.label_h = 15 if filename else 0
        self.code_h = len(self.lines) * self.leading + 2 * self.pad
        self.width = self.max_width
        self.height = self.label_h + self.code_h

    def wrap(self, availWidth, availHeight):
        self.width = min(self.max_width, availWidth)
        return (self.width, self.height)

    def draw(self):
        c = self.canv
        y = self.height

        if self.filename:
            y -= self.label_h
            c.setFillColor(COLORS['accent2'])
            c.roundRect(0, y, self.width, self.label_h, 2.5, fill=1, stroke=0)
            c.setFillColor(COLORS['white'])
            c.setFont('Courier-Bold', 7.5)
            c.drawString(8, y + 3.5, self.filename)

        # Code background
        c.setFillColor(COLORS['bg_code'])
        c.setStrokeColor(COLORS['border_light'])
        c.roundRect(0, 0, self.width, y, 2.5, fill=1, stroke=1)

        # Gutter
        c.setFillColor(HexColor('#e8e8f0'))
        c.roundRect(0, 0, self.gutter_w, y, 2.5, fill=1, stroke=0)

        text_y = y - self.pad - self.font_size
        for i, line in enumerate(self.lines):
            line_num = self.start_line + i

            c.setFillColor(COLORS['text_secondary'])
            c.setFont('Courier', self.font_size - 1)
            c.drawRightString(self.gutter_w - 4, text_y, str(line_num))

            c.setFillColor(COLORS['text_code'])
            c.setFont('Courier', self.font_size)

            max_chars = int((self.width - self.gutter_w - 10) /
                            (self.font_size * 0.60))
            display = line if len(line) <= max_chars else line[:max_chars-3] + '...'
            c.drawString(self.gutter_w + 5, text_y, display)

            text_y -= self.leading

    def split(self, availWidth, availHeight):
        if availHeight >= self.height:
            return [self]

        # Need at least label + padding + 3 lines to bother splitting;
        # otherwise signal Platypus to page-break and retry at full height.
        min_h = self.label_h + 2 * self.pad + 3 * self.leading
        if availHeight < min_h:
            return []

        usable = availHeight - self.label_h - 2 * self.pad
        lines_fit = max(1, int(usable / self.leading) - 1)  # 1-line safety margin
        if lines_fit >= len(self.lines):
            return [self]

        first_code = '\n'.join(self.lines[:lines_fit])
        rest_code = '\n'.join(self.lines[lines_fit:])
        first = CodeBlock(first_code, self.filename,
                          self.start_line, self.font_size, self.leading,
                          max_width=self.max_width)
        rest = CodeBlock(
            rest_code,
            f"{self.filename} (continued)" if self.filename else None,
            self.start_line + lines_fit, self.font_size, self.leading,
            max_width=self.max_width)
        return [first, rest]


# ---------------------------------------------------------------------------
# StructCard flowable
# ---------------------------------------------------------------------------

class StructCard(Flowable):
    def __init__(self, struct_name, fields, width=None):
        super().__init__()
        self.struct_name = struct_name
        self.fields = fields  # list of (type, name, description)
        self.width = width or CONTENT_W
        self.header_h = 20
        self.row_h = 15
        self.pad = 0
        self.height = self.header_h + len(self.fields) * self.row_h + self.pad

    def wrap(self, availWidth, availHeight):
        self.width = min(self.width, availWidth)
        return (self.width, self.height)

    def draw(self):
        c = self.canv
        y = self.height

        # Header
        y -= self.header_h
        c.setFillColor(COLORS['accent2'])
        c.roundRect(0, y, self.width, self.header_h, 3, fill=1, stroke=0)
        c.setFillColor(COLORS['white'])
        c.setFont('Courier-Bold', 9.5)
        c.drawString(8, y + 5.5, f"struct {self.struct_name}")

        col_type_w = self.width * 0.30
        col_name_w = self.width * 0.27

        for i, (ftype, fname, fdesc) in enumerate(self.fields):
            row_y = y - (i + 1) * self.row_h
            bg = COLORS['bg_struct'] if i % 2 == 0 else COLORS['bg_struct_alt']
            c.setFillColor(bg)
            c.rect(0, row_y, self.width, self.row_h, fill=1, stroke=0)

            c.setFillColor(COLORS['text_type'])
            c.setFont('Courier', 7.5)
            c.drawString(8, row_y + 4, ftype[:34])

            c.setFillColor(COLORS['text_primary'])
            c.setFont('Courier-Bold', 7.5)
            c.drawString(col_type_w + 4, row_y + 4, fname[:26])

            c.setFillColor(COLORS['text_secondary'])
            c.setFont('Helvetica', 7.2)
            c.drawString(col_type_w + col_name_w + 4, row_y + 4, fdesc[:76])

        # Border
        c.setStrokeColor(COLORS['border_light'])
        c.roundRect(0, y - len(self.fields) * self.row_h,
                    self.width, len(self.fields) * self.row_h + self.header_h,
                    3, fill=0, stroke=1)


# ---------------------------------------------------------------------------
# Flow / relationship diagrams
# ---------------------------------------------------------------------------

STAGE_COLORS = [
    '#0f3460', '#16213e', '#533483', '#e94560', '#0f766e',
    '#1a1a2e', '#b91c1c', '#0369a1', '#65a30d',
]


def flow_diagram(stages, title="", width=CONTENT_W, box_h=28):
    """Vertical flow: boxes connected by down-arrows. stages: list of str."""
    padding = 12
    arrow_h = 18
    box_w = width - 2 * padding
    total_h = len(stages) * (box_h + arrow_h) - arrow_h + (30 if title else 0) + 20

    d = Drawing(width, total_h)

    y = total_h - 10
    if title:
        d.add(String(width / 2, y, title, fontSize=11,
                     fontName='Helvetica-Bold',
                     textAnchor='middle', fillColor=HexColor('#1a1a2e')))
        y -= 22

    for i, stage in enumerate(stages):
        color = HexColor(STAGE_COLORS[i % len(STAGE_COLORS)])
        d.add(Rect(padding, y - box_h, box_w, box_h,
                   fillColor=color, strokeColor=None, rx=5))
        d.add(String(width / 2, y - box_h / 2 - 3, stage,
                     fontSize=9, fontName='Helvetica-Bold',
                     textAnchor='middle', fillColor=HexColor('#ffffff')))
        y -= box_h

        if i < len(stages) - 1:
            mid_x = width / 2
            d.add(Line(mid_x, y, mid_x, y - arrow_h + 6,
                       strokeColor=HexColor('#7a7a9a'), strokeWidth=1.3))
            d.add(Polygon(
                points=[mid_x - 4, y - arrow_h + 8,
                        mid_x + 4, y - arrow_h + 8,
                        mid_x, y - arrow_h + 2],
                fillColor=HexColor('#7a7a9a'), strokeColor=None))
            y -= arrow_h

    return d


def relationship_diagram(nodes, edges, width=CONTENT_W):
    """Grid of struct boxes connected by arrows.

    nodes: dict of name -> (col, row) position in grid units.
    edges: list of (src_name, dst_name) pairs — arrow src -> dst.
    """
    box_w, box_h = 115, 26
    col_gap, row_gap = 22, 30

    cols = max(c for c, _ in nodes.values()) + 1
    rows = max(r for _, r in nodes.values()) + 1
    total_w = cols * box_w + (cols - 1) * col_gap + 2 * 10
    total_h = rows * box_h + (rows - 1) * row_gap + 20

    d = Drawing(min(total_w, width), total_h)

    pos = {}
    for name, (col, row) in nodes.items():
        x = 10 + col * (box_w + col_gap)
        y = total_h - 10 - row * (box_h + row_gap) - box_h
        pos[name] = (x, y)

        d.add(Rect(x, y, box_w, box_h,
                   fillColor=COLORS['bg_struct'],
                   strokeColor=COLORS['accent2'], strokeWidth=1, rx=3))
        d.add(String(x + box_w / 2, y + box_h / 2 - 3, name,
                     fontSize=7.8, fontName='Courier-Bold',
                     textAnchor='middle', fillColor=COLORS['text_primary']))

    for src, dst in edges:
        if src not in pos or dst not in pos:
            continue
        sx, sy = pos[src]
        dx, dy = pos[dst]
        sx_cx = sx + box_w / 2
        sy_cy = sy + box_h / 2
        dx_cx = dx + box_w / 2
        dy_cy = dy + box_h / 2

        # Prefer edges along column if same col, else side-to-side
        if abs(sx_cx - dx_cx) < 10:
            # vertical
            if sy > dy:
                y_top = sy
                y_bot = dy + box_h
            else:
                y_top = sy + box_h
                y_bot = dy
            d.add(Line(sx_cx, y_top, sx_cx, y_bot,
                       strokeColor=COLORS['accent'], strokeWidth=1))
            # arrow head at dst
            if sy > dy:
                d.add(Polygon(
                    points=[sx_cx - 3, y_bot + 5,
                            sx_cx + 3, y_bot + 5,
                            sx_cx, y_bot],
                    fillColor=COLORS['accent'], strokeColor=None))
            else:
                d.add(Polygon(
                    points=[sx_cx - 3, y_bot - 5,
                            sx_cx + 3, y_bot - 5,
                            sx_cx, y_bot],
                    fillColor=COLORS['accent'], strokeColor=None))
        else:
            # horizontal-ish; draw a polyline
            # from right of src to left of dst (or reverse)
            if sx_cx < dx_cx:
                x_start = sx + box_w
                x_end = dx
                arrow_x = x_end
            else:
                x_start = sx
                x_end = dx + box_w
                arrow_x = x_end
            d.add(Line(x_start, sy_cy, x_end, dy_cy,
                       strokeColor=COLORS['accent'], strokeWidth=1))
            if sx_cx < dx_cx:
                d.add(Polygon(
                    points=[x_end - 5, dy_cy + 3,
                            x_end - 5, dy_cy - 3,
                            x_end, dy_cy],
                    fillColor=COLORS['accent'], strokeColor=None))
            else:
                d.add(Polygon(
                    points=[x_end + 5, dy_cy + 3,
                            x_end + 5, dy_cy - 3,
                            x_end, dy_cy],
                    fillColor=COLORS['accent'], strokeColor=None))

    return d


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def P(text, styles, style='BodyText2'):
    """Shortcut for a Paragraph with auto-escaping of &/</>."""
    text = text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    return Paragraph(text, styles[style])


def Praw(text, styles, style='BodyText2'):
    """Paragraph without escaping — use when you intentionally use XML tags."""
    return Paragraph(text, styles[style])


def bullet(text, styles):
    text = text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    return Paragraph('&bull;&nbsp; ' + text, styles['BulletText'])


# ---------------------------------------------------------------------------
# Footer
# ---------------------------------------------------------------------------

def footer(canvas, doc):
    canvas.saveState()
    canvas.setFont('Helvetica', 7.5)
    canvas.setFillColor(COLORS['text_secondary'])

    pn = canvas.getPageNumber()
    canvas.drawRightString(PAGE_W - MARGIN_RIGHT, 30, f"Page {pn}")
    canvas.drawString(MARGIN_LEFT, 30,
                      "humo2 — Frame Lifecycle Reference")

    canvas.setStrokeColor(COLORS['border_light'])
    canvas.setLineWidth(0.5)
    canvas.line(MARGIN_LEFT, 42, PAGE_W - MARGIN_RIGHT, 42)
    canvas.restoreState()


def title_page_bg(canvas, doc):
    # Dark background for page 1 only
    canvas.saveState()
    canvas.setFillColor(COLORS['bg_dark'])
    canvas.rect(0, 0, PAGE_W, PAGE_H, fill=1, stroke=0)
    # Accent bar
    canvas.setFillColor(COLORS['accent'])
    canvas.rect(0, PAGE_H - 180, PAGE_W, 6, fill=1, stroke=0)
    canvas.restoreState()


# ---------------------------------------------------------------------------
# Content assembly
# ---------------------------------------------------------------------------

def build_story(styles):
    story = []

    # -------------------------------------------------------------
    # TITLE PAGE
    # -------------------------------------------------------------
    story.append(Spacer(1, 140))
    story.append(Paragraph("humo2", styles['DocTitle']))
    story.append(Spacer(1, 6))
    story.append(Paragraph("Frame Lifecycle Reference",
                           styles['DocSubtitle']))
    story.append(Spacer(1, 30))
    story.append(Paragraph(
        "A technical walkthrough of the per-frame hot path in the humo2 "
        "Vulkan renderer — scene data structures, transform resolve, "
        "frustum cull, LOD-partitioned SSBO fill, and render-graph execute.",
        styles['TitleMeta']))
    story.append(Spacer(1, 260))
    story.append(Paragraph(
        "C17 engine core &middot; Vulkan 1.3 &middot; cglm",
        styles['TitleMeta']))
    story.append(Paragraph(
        "Target: single developer, pedagogical reference",
        styles['TitleMeta']))
    story.append(PageBreak())

    # -------------------------------------------------------------
    # TOC
    # -------------------------------------------------------------
    story.append(Paragraph("Table of Contents", styles['SectionH1']))
    toc = [
        ('1', "Frame Lifecycle Overview",                'TOCLineBold'),
        ('',  "1.1 What drawFrame does",                 'TOCLine'),
        ('',  "1.2 Hot-path stage diagram",              'TOCLine'),
        ('2', "Scene Data Structures (Hot Path)",        'TOCLineBold'),
        ('',  "2.1 SCENE — top-level container",         'TOCLine'),
        ('',  "2.2 TransformManager — SoA preorder arrays", 'TOCLine'),
        ('',  "2.3 Models3DManager and Model3D",         'TOCLine'),
        ('',  "2.4 LightManager and LightVolume",        'TOCLine'),
        ('',  "2.5 ParticleGroupManager",                'TOCLine'),
        ('',  "2.6 Camera3D and FrameGlobals",           'TOCLine'),
        ('',  "2.7 UBO layouts: ModelInstanceUBO, LightInstanceUBO", 'TOCLine'),
        ('',  "2.8 AABB, FrustumPlanes, ModelLOD",       'TOCLine'),
        ('',  "2.9 Struct relationships at a glance",    'TOCLine'),
        ('3', "drawFrame() — the orchestrator",           'TOCLineBold'),
        ('4', "Stage-by-Stage Walkthrough",              'TOCLineBold'),
        ('',  "4.1 Timeline semaphore wait (CPU pacing)", 'TOCLine'),
        ('',  "4.2 Deferred destruction flush",          'TOCLine'),
        ('',  "4.3 Transform hierarchy resolve",         'TOCLine'),
        ('',  "4.4 Frame globals (view / proj / viewProj)", 'TOCLine'),
        ('',  "4.5 Frustum plane extraction + cull",     'TOCLine'),
        ('',  "4.6 Model UBO fill with LOD partitioning", 'TOCLine'),
        ('',  "4.7 Light UBO fill",                      'TOCLine'),
        ('',  "4.8 Command recording + staging copies",  'TOCLine'),
        ('',  "4.9 Render graph execute + pass callbacks", 'TOCLine'),
        ('',  "4.10 Submit + present",                   'TOCLine'),
        ('5', "Performance Envelope",                    'TOCLineBold'),
    ]
    for num, title, stl in toc:
        if num:
            story.append(Paragraph(f"{num}. {title}", styles[stl]))
        else:
            story.append(Paragraph(title, styles[stl]))
    story.append(PageBreak())

    # =============================================================
    # 1. FRAME LIFECYCLE OVERVIEW
    # =============================================================
    story.append(Paragraph("1. Frame Lifecycle Overview",
                            styles['SectionH1']))

    story.append(Paragraph("1.1 What drawFrame does",
                            styles['SectionH2']))
    story.append(P(
        "Every visible frame in humo2 begins and ends inside one call to "
        "<b>drawFrame()</b> in <font face=\"Courier\">src/engine/display/display.c</font>. "
        "That function orchestrates the complete CPU-side scene "
        "traversal (transform resolve, frustum cull, per-instance LOD "
        "classification, UBO writes), coordinates timeline-semaphore "
        "synchronization with the GPU, records command buffers, and "
        "submits them on the graphics + compute queues before presenting "
        "the swapchain image.", styles, 'BodyText2'))
    story.append(P(
        "This document walks the entire hot path. Every struct that "
        "<b>drawFrame</b> touches every frame is described in Section 2; "
        "every stage of <b>drawFrame</b>'s body is walked in Section 4 "
        "with the actual C source inline.", styles))

    story.append(Paragraph("1.2 Hot-path stage diagram",
                            styles['SectionH2']))

    story.append(flow_diagram([
        "CPU: wait on timeline semaphore (frame N-2 finished)",
        "CPU: flush deferred-destruction queue for freed slot",
        "CPU: acquire next swapchain image",
        "CPU: resolveTransforms — preorder SoA walk, dirty-flag skip",
        "CPU: updateFrameGlobals — view, proj, viewProj, invViewProj",
        "CPU: frustumExtractPlanes + frustumCullScene (bulk-skip)",
        "CPU: updateModelUniformBuffersDirect — LOD partition + SSBO write",
        "CPU: updateLightUniformBuffersDirect",
        "CPU: vkCmdCopyBuffer staging -> device-local (model + light)",
        "CPU: rgExecute — render graph records all pass callbacks",
        "CPU: rgSubmit — graphics + compute with timeline sem sync",
        "CPU: vkQueuePresentKHR",
    ], title="Per-frame stages of drawFrame()"))
    story.append(Paragraph(
        "Figure 1.1 — The twelve stages of every frame, in order. "
        "Timing instrumentation in DEBUG builds brackets each stage with "
        "MARK_TIME to produce the per-scenario bench output.",
        styles['Caption']))
    story.append(PageBreak())

    # =============================================================
    # 2. SCENE DATA STRUCTURES
    # =============================================================
    story.append(Paragraph("2. Scene Data Structures (Hot Path)",
                            styles['SectionH1']))
    story.append(P(
        "The structs in this section are every type that <b>drawFrame</b> "
        "reads or writes on the hot path. Cold-path fields (one-time "
        "load-time staging, debug info, asset metadata) are omitted unless "
        "they carry semantics that matter for per-frame execution.",
        styles))

    # -------- 2.1 SCENE
    story.append(Paragraph("2.1 SCENE — top-level container",
                            styles['SectionH2']))
    story.append(P(
        "<b>SCENE</b> is the root aggregate that every hot-path function "
        "receives. It owns the transform manager (authoritative per-frame "
        "state), three per-type managers (models, lights, particles), and "
        "an embedded camera copy. The camera is an embedded value type "
        "synchronized from the C++ Scene wrapper each frame via direct "
        "struct assignment in drawFrame.", styles))
    story.append(StructCard("SCENE", [
        ("TransformManager",         "transformManager",   "preorder hot/world/cold SoA arrays; authoritative per-frame state"),
        ("LightManager",             "lightManager",       "light types + dense instance array + packed geometry"),
        ("Models3DManager",          "modelsManager",      "model types + dense instance array + packed LOD index buffer"),
        ("uint32_t",                 "sceneInstancesLen",  "total live instance count (models + lights), set at init"),
        ("ParticleGroupManager",     "particleManager",    "particle groups + GPU compute simulation state"),
        ("uint32_t",                 "particleGroupsLen",  "number of particle groups"),
        ("Camera3D",                 "camera",             "eye/center/up/screenSize; synced from wrapper in drawFrame"),
    ]))

    # -------- 2.2 TransformManager
    story.append(Paragraph("2.2 TransformManager — SoA preorder arrays",
                            styles['SectionH2']))
    story.append(P(
        "The single most performance-critical structure in the engine. "
        "Holds the preorder-flattened scene graph as three parallel arrays "
        "so <b>resolveTransforms</b>, <b>frustumCullScene</b>, and the UBO "
        "fills all traverse in strictly sequential order with dirty-flag "
        "skip. The hot array is 12 B per entry so 20K transforms fit in "
        "~240 KB (L2-resident on most CPUs); the world matrix array is "
        "parallel and only loaded when a node is dirty.",
        styles))
    story.append(StructCard("TransformHot (12 B)", [
        ("uint32_t", "parentIdx",  "preorder index of parent, or TRANSFORM_NO_PARENT for roots"),
        ("uint16_t", "lifecycle",  "TRANSFORM_ALIVE / DISABLED / DESTROYED"),
        ("uint16_t", "passFlags",  "bit 0 DISABLED (per-frame), bit 1 DIRTY (sticky)"),
        ("uint32_t", "skipCount",  "size of this subtree; O(1) bulk-skip on cull"),
    ]))
    story.append(StructCard("TransformWorld (64 B)", [
        ("mat4", "worldMatrix", "one full cache line; written by resolve dirty branch, read by cull + UBO fill"),
    ]))
    story.append(StructCard("TransformCold (48 B)", [
        ("vec3",   "position",  "translation + 4 B pad"),
        ("versor", "rotation",  "quaternion, 16 B aligned for SSE"),
        ("vec3",   "scale",     "uniform or per-axis + 4 B pad"),
    ]))
    story.append(StructCard("TransformManager", [
        ("TransformHot*",   "pTransformsHot",    "[count] 12 B loop state — walked every frame"),
        ("TransformWorld*", "pTransformsWorld",  "[count] 64 B world matrix — lazy read"),
        ("TransformCold*",  "pTransformsCold",   "[count] TRS source — read only on dirty branch"),
        ("PreorderEntity*", "pPreorderEntities", "[count] reverse index -> type + instance"),
        ("AABB*",           "pLocalAABBs",       "[count] per-entity local AABB for cull"),
        ("uint32_t",        "count",             "total allocated slots"),
        ("uint32_t",        "liveCount",         "updated per frame by resolve pass"),
    ]))

    story.append(StructCard("PreorderEntity (8 B)", [
        ("uint16_t", "kind",     "SCENE_ENTITY_MODEL (0) or SCENE_ENTITY_LIGHT (1)"),
        ("uint16_t", "typeIdx",  "index into pModel[] or pLight[]"),
        ("uint32_t", "denseIdx", "index into pDenseModelInstances[] or pDenseLightInstances[]"),
    ]))
    story.append(Spacer(1, 6))
    story.append(PageBreak())

    # -------- 2.3 Models3DManager + Model3D
    story.append(Paragraph("2.3 Models3DManager and Model3D",
                            styles['SectionH2']))
    story.append(P(
        "Instances are stored dense across all types in "
        "<b>pDenseModelInstances</b>; each <b>Model3D</b> owns a contiguous "
        "range identified by <b>denseBaseOffset</b> and "
        "<b>instanceCount</b>. Per-LOD index ranges live in "
        "<b>packedModelIndex</b> with offsets tracked in the "
        "<b>lod[MODEL_LOD_COUNT]</b> array. LOD 0 shares the same vertex "
        "range as the asset's unrolled mesh; LOD 1 and LOD 2 append "
        "simplified index ranges produced at load time by "
        "meshoptimizer.", styles))
    story.append(StructCard("Models3DManager", [
        ("Model3D*",         "pModel",                    "array of model types"),
        ("uint32_t",         "modelLen",                  "number of model types"),
        ("uint32_t",         "modelInstanceCapacity",     "total instance slots (immutable after init)"),
        ("uint32_t",         "modelInstanceLen",          "live instance count — set by UBO fill"),
        ("Model3DInstance*", "pDenseModelInstances",      "dense allocation, indexed via denseBaseOffset"),
        ("MODEL_VERTEX*",    "pPackedModelVertexLean",    "packed 32 B lean vertex buffer"),
        ("ArrayUint32_t",    "packedModelIndex",          "packed index buffer — all LODs appended"),
        ("ArrayUint32_t",    "instanceOffset",            "[modelLen] per-type SSBO base offsets"),
    ]))
    story.append(StructCard("Model3D", [
        ("Mesh3DManager", "meshManager",            "source mesh list (load-time only)"),
        ("ArrayUint32_t", "indices",                "per-model indices (load-time)"),
        ("uint32_t",      "vertexOffset",           "base offset into packed vertex buffer"),
        ("uint32_t",      "indiceOffset",           "base offset into packed index buffer (LOD 0)"),
        ("uint32_t",      "instanceCount",          "capacity; immutable after init"),
        ("uint32_t",      "liveInstanceCount",      "per-frame live count — sum of lodLiveInstanceCount"),
        ("uint32_t",      "alive",                  "MODEL_ALIVE (1), DISABLED (2), DESTROYED (0)"),
        ("uint32_t",      "denseBaseOffset",        "base index into pDenseModelInstances"),
        ("AABB",          "localAABB",              "local-space bounding box (cull input)"),
        ("float",         "localAABBRadius",        "half-diagonal; LOD selection input"),
        ("ModelLOD",      "lod[MODEL_LOD_COUNT]",   "3 LOD tiers: full, medium, coarse"),
        ("uint32_t",      "lodLiveInstanceCount[]", "per-LOD live count — fill output, draw input"),
    ]))
    story.append(StructCard("Model3DInstance (~24 B)", [
        ("GEOM_VERTEX_COLOR_RGBA", "color",        "RGBA per-instance color"),
        ("uint32_t",               "transformIdx", "index into TransformManager.pTransformsWorld"),
        ("uint16_t",               "lifecycle",    "INSTANCE_ALIVE / DISABLED / DESTROYED"),
        ("uint16_t",               "passFlags",    "reserved for hierarchy cascade-disable"),
    ]))
    story.append(StructCard("ModelLOD (20 B)", [
        ("uint32_t", "vertexOffset", "offset into packed lean vertex buffer"),
        ("uint32_t", "vertexCount",  "upper-bound vertex count for this LOD"),
        ("uint32_t", "indexOffset",  "offset into packed index buffer"),
        ("uint32_t", "indexCount",   "triangle count * 3 for this LOD"),
        ("float",    "minPixelSize", "threshold: pick this LOD when pixelSize >= this"),
    ]))
    story.append(PageBreak())

    # -------- 2.4 LightManager + LightVolume
    story.append(Paragraph("2.4 LightManager and LightVolume",
                            styles['SectionH2']))
    story.append(P(
        "Mirrors the model manager pattern for light volumes. Each "
        "<b>LightVolume</b> carries a <b>Model3D</b> for its proxy "
        "geometry (typically a cube or sphere). The light UBO fill "
        "writes an extended instance struct that includes per-instance "
        "color, world-space position, and uniform scale — the latter "
        "used in the fragment shader for attenuation falloff.", styles))
    story.append(StructCard("LightManager", [
        ("LightVolume*",         "pLight",                "light type array"),
        ("uint32_t",             "lightLen",              "number of light types"),
        ("LightVolumeInstance*", "pDenseLightInstances",  "dense allocation of light instances"),
        ("uint32_t",             "lightInstanceCapacity", "total slots (immutable after init)"),
        ("uint32_t",             "lightInstanceCount",    "live instance count — per-frame"),
        ("ArrayUint32_t",        "packedLightIndex",      "packed light proxy indices"),
        ("ArrayUint32_t",        "instanceOffset",        "[lightLen] per-type SSBO base offsets"),
    ]))
    story.append(StructCard("LightVolume", [
        ("LightProperties", "properties",               "transparency, intensity, falloff"),
        ("Model3D",         "model",                    "proxy geometry (cube/sphere)"),
        ("uint32_t",        "denseBaseOffset",          "base offset into pDenseLightInstances"),
        ("uint32_t",        "lightVolumeInstanceCount", "capacity"),
        ("uint32_t",        "liveInstanceCount",        "per-frame live count"),
        ("uint32_t",        "alive",                    "LIGHT_ALIVE / DISABLED / DESTROYED"),
        ("AABB",            "localAABB",                "local-space AABB for cull"),
    ]))
    story.append(StructCard("LightVolumeInstance", [
        ("GEOM_VERTEX_COLOR_RGBA", "color",         "RGBA per-instance color"),
        ("uint32_t",               "transformIdx",  "index into TransformManager"),
        ("uint16_t",               "lifecycle",     "alive / disabled / destroyed"),
        ("uint16_t",               "passFlags",     "reserved for cascade disable"),
    ]))

    # -------- 2.5 Particles
    story.append(Paragraph("2.5 ParticleGroupManager",
                            styles['SectionH2']))
    story.append(P(
        "Particles are simulated on the GPU by a compute pipeline, then "
        "rasterized in the graphics queue. The CPU side of "
        "<b>drawFrame</b> does not touch per-particle data directly; it "
        "only updates SSBO descriptor sets and lets the compute pass run "
        "as part of the render graph dispatch.", styles))
    story.append(StructCard("ParticleGroupManager", [
        ("ParticleGroup*",    "pParticleGroup",           "group array"),
        ("uint32_t",          "particleGroupLen",         "number of groups"),
        ("ParticleInstance*", "pParticleInstance",        "dense packed per-particle data"),
        ("uint32_t",          "packedParticleInstanceLen","total packed particle count"),
        ("TrzVertex*",        "pPackedParticleVertex",    "billboard quad vertex buffer"),
        ("ArrayUint32_t",     "particleInstanceOffsets",  "per-group SSBO base offsets"),
    ]))
    story.append(StructCard("ParticleInstance (48 B)", [
        ("vec4", "position", "xyz world position + pad"),
        ("vec4", "velocity", "xyz velocity + pad"),
        ("vec4", "color",    "RGBA"),
    ]))

    story.append(PageBreak())

    # -------- 2.6 Camera3D + FrameGlobals
    story.append(Paragraph("2.6 Camera3D and FrameGlobals",
                            styles['SectionH2']))
    story.append(P(
        "<b>Camera3D</b> is passed by value into drawFrame from the C++ "
        "Scene wrapper. It's also embedded in SCENE (updated each frame) "
        "because <b>updateModelUniformBuffersDirect</b> needs "
        "<b>screenSize</b> and <b>eye</b> for LOD pixel-size calculation. "
        "<b>FrameGlobals</b> is the UBO at binding 9 shared across every "
        "pipeline that needs view/projection matrices — written once per "
        "frame, read by every vertex shader.", styles))
    story.append(StructCard("Camera3D (std140)", [
        ("vec3",      "eye",         "world-space camera origin + pad"),
        ("vec3",      "center",      "look-at point + pad"),
        ("vec3",      "up",          "world-space up vector + pad"),
        ("vec2",      "screenSize",  "viewport pixel extent + 8 B pad"),
    ]))
    story.append(StructCard("FrameGlobals (UBO binding 9)", [
        ("mat4",     "view",        "lookAt(eye, center, up)"),
        ("mat4",     "proj",        "perspective(45deg, aspect, 0.1, 2000) with Y-flip"),
        ("mat4",     "viewProj",    "precomputed proj * view"),
        ("mat4",     "invViewProj", "for light fragment world-pos reconstruction"),
        ("Camera3D", "camera",      "embedded — eye/center/up/screenSize"),
    ]))

    # -------- 2.7 UBO layouts
    story.append(Paragraph("2.7 UBO layouts: ModelInstanceUBO, LightInstanceUBO",
                            styles['SectionH2']))
    story.append(P(
        "Per-instance SSBO layouts consumed by the model and light "
        "vertex shaders. Indexed by <font face=\"Courier\">gl_InstanceIndex "
        "+ firstInstance</font>, where <b>firstInstance</b> for each "
        "draw is <i>typeBase + lodOffset</i> — this is how the LOD "
        "partition flows from CPU classification to GPU draw.", styles))
    story.append(StructCard("ModelInstanceUBO (64 B)", [
        ("mat4", "model", "world matrix from TransformWorld.worldMatrix"),
    ]))
    story.append(StructCard("LightInstanceUBO (~88 B)", [
        ("mat4",                   "model",    "world matrix"),
        ("GEOM_VERTEX_COLOR_RGBA", "color",    "RGBA"),
        ("vec3",                   "position", "world-space position"),
        ("float",                  "scale",    "uniform scale (used for radius/falloff)"),
    ]))

    # -------- 2.8 AABB + Frustum + ModelLOD (already shown, but for completeness)
    story.append(Paragraph("2.8 AABB and FrustumPlanes",
                            styles['SectionH2']))
    story.append(P(
        "Two tiny structs that underpin the cull path. <b>AABB</b> is "
        "min/max corners in local space; the cull transforms it to "
        "world space via Arvo's method (no 8-corner iteration). "
        "<b>FrustumPlanes</b> holds six (a,b,c,d) coefficients "
        "extracted Gribb/Hartmann-style from viewProj; each plane is "
        "normalized so the signed-distance p-vertex test is a straight "
        "dot product.", styles))
    story.append(StructCard("AABB (24 B)", [
        ("vec3", "min", "per-axis minimum"),
        ("vec3", "max", "per-axis maximum"),
    ]))
    story.append(StructCard("FrustumPlanes (96 B)", [
        ("vec4", "planes[0]", "left   = row3 + row0 (normalized)"),
        ("vec4", "planes[1]", "right  = row3 - row0"),
        ("vec4", "planes[2]", "bottom = row3 + row1"),
        ("vec4", "planes[3]", "top    = row3 - row1"),
        ("vec4", "planes[4]", "near   = row2  (Vulkan [0,1] depth)"),
        ("vec4", "planes[5]", "far    = row3 - row2"),
    ]))

    story.append(PageBreak())

    # -------- 2.9 Relationship diagram
    story.append(Paragraph("2.9 Struct relationships at a glance",
                            styles['SectionH2']))
    story.append(P(
        "Arrows go from containing type to contained type, or from a "
        "reference holder to the referenced type. Every struct in this "
        "diagram is read or written by drawFrame on every frame.",
        styles))
    nodes = {
        # col, row
        'SCENE':                 (1, 0),
        'TransformManager':      (0, 1),
        'Models3DManager':       (1, 1),
        'LightManager':          (2, 1),
        'Camera3D':              (1, 2),
        'TransformHot':          (0, 3),
        'TransformWorld':        (0, 4),
        'Model3D':               (1, 3),
        'Model3DInstance':       (1, 4),
        'LightVolume':           (2, 3),
        'LightVolumeInstance':   (2, 4),
        'PreorderEntity':        (0, 5),
        'AABB':                  (1, 5),
        'ModelLOD':              (2, 5),
    }
    edges = [
        ('SCENE', 'TransformManager'),
        ('SCENE', 'Models3DManager'),
        ('SCENE', 'LightManager'),
        ('SCENE', 'Camera3D'),
        ('TransformManager', 'TransformHot'),
        ('TransformManager', 'TransformWorld'),
        ('TransformManager', 'PreorderEntity'),
        ('TransformManager', 'AABB'),
        ('Models3DManager', 'Model3D'),
        ('Models3DManager', 'Model3DInstance'),
        ('Model3D', 'ModelLOD'),
        ('Model3D', 'AABB'),
        ('LightManager', 'LightVolume'),
        ('LightManager', 'LightVolumeInstance'),
        ('LightVolume', 'AABB'),
    ]
    story.append(relationship_diagram(nodes, edges))
    story.append(Paragraph(
        "Figure 2.1 — Hot-path struct hierarchy. Every arrow is a "
        "pointer or embedded value that drawFrame follows each frame.",
        styles['Caption']))
    story.append(PageBreak())

    # =============================================================
    # 3. drawFrame() source
    # =============================================================
    story.append(Paragraph("3. drawFrame() — the orchestrator",
                            styles['SectionH1']))
    story.append(P(
        "The full <b>drawFrame</b> function from "
        "<font face=\"Courier\">src/engine/display/display.c</font>. "
        "Section 4 walks each annotated block one by one. Line numbers "
        "match the source.", styles))

    drawframe_code = r'''void drawFrame(PhysicalDevice* pd, LogicalDevice* ld, DisplayManager* dm, GraphicPipelineToSurface* gdp,
               GraphicPipelineToSurface* particleGraphics, GraphicPipelineToSurface* lightGraphics, VkDevice device,
               SDL_Window* window, float deltaTime, Camera3D camera, RenderGraph* rg)
{
#ifdef DEBUG
    MARK_TIME(tFrameStart);
#endif

    FrameSync* sync = &ld->frameSync;

    // CPU pacing: wait for frame N-maxFramesInFlight to finish on the graphics timeline
    if (sync->graphicsValue >= sync->maxFramesInFlight)
    {
        uint64_t            waitValue = sync->graphicsValue - sync->maxFramesInFlight + 1;
        VkSemaphoreWaitInfo waitInfo  = {
            .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores    = &sync->graphicsTimeline,
            .pValues        = &waitValue,
        };
        VkResult waitResult = vkWaitSemaphores(device, &waitInfo, 100000000ULL); // 100ms
        if (waitResult == VK_TIMEOUT)
            return;
    }

    // Deferred destruction: any resource enqueued >= maxFramesInFlight frames
    // ago is now GPU-idle and safe to destroy.
    flushDeletionQueue(&ld->deletionQueue, ld->device, ld->allocator);
    advanceDeletionQueue(&ld->deletionQueue);

    // Descriptor set update: particle compute reads previous-frame SSBO and
    // writes current-frame SSBO; rebind each frame before recording.
    updateSSBODescriptorSets(ld->device, particleGraphics, dm->currentFrame, particleGraphics->maxFrames);

    uint32_t aquireImageIndex = dm->currentFrame;
    VkResult nextImageResult =
        vkAcquireNextImageKHR(device, dm->graphicPipeline->swapchain->VkSwapchain, 100000000ULL,
                              gdp->imgViewSemaphore[dm->currentFrame], VK_NULL_HANDLE, &aquireImageIndex);
    // (swapchain recreation omitted for brevity)

    // Resolve transform hierarchy — writes to TransformWorld.worldMatrix for dirty nodes.
    resolveTransforms(&dm->graphicPipeline[0].scene->transformManager);

    // Build FrameGlobals UBO (view + proj + viewProj + invViewProj + camera).
    FrameGlobals globals;
    updateFrameGlobals(&globals, dm->graphicPipeline->swapchain, camera);
    memcpy(dm->graphicPipeline[0].pGlobalsUbo->uniformBuffersMapped[dm->currentFrame], &globals,
           sizeof(FrameGlobals));

    // Frustum cull — marks TRANSFORM_PASS_DISABLED on instances outside the view frustum.
    FrustumPlanes frustum;
    frustumExtractPlanes(globals.viewProj, &frustum);
    frustumCullScene(dm->graphicPipeline[0].scene, &frustum);

    // Sync live camera into the C SCENE struct so LOD classifier sees screenSize + eye.
    // updateModelUniformBuffersDirect writes ModelInstanceUBO per live instance,
    // partitioned by LOD within each type's SSBO range.
    dm->graphicPipeline[0].scene->camera = camera;
    updateModelUniformBuffersDirect(gdp->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame],
                                    dm->graphicPipeline[0].scene);
#ifdef DEBUG
    displayBenchRecordLodSample(dm->graphicPipeline[0].scene);
#endif

    // Light SSBO fill
    updateLightUniformBuffersDirect(lightGraphics->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame],
                                    deltaTime, dm->graphicPipeline[2].scene);

    // Command recording
    VkCommandBuffer graphicsCB = gdp->commandBuffer[dm->currentFrame];
    VkCommandBuffer computeCB  = particleGraphics->computeP->commandBuffer[dm->currentFrame];
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(graphicsCB, &beginInfo);
    vkBeginCommandBuffer(computeCB, &beginInfo);

    // Staging -> device-local SSBO copies for model + light instance buffers.
    // (Full barrier code shown in Section 4.8)

    // Update per-frame resource handles (swapchain image + particle SSBO pair).
    rg->resources[0].image     = gdp->swapchain->p_scImages[dm->currentFrame];
    rg->resources[0].imageView = gdp->swapchain->p_scImageView[dm->currentFrame];
    uint32_t prevFrame = (dm->currentFrame - 1 + particleGraphics->maxFrames) % particleGraphics->maxFrames;
    rg->resources[4].buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[prevFrame];
    rg->resources[5].buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[dm->currentFrame];
    rg->deltaTime = deltaTime;

    // Execute the compiled render graph: records each pass's execute callback
    // with auto-inserted barriers.
    rgExecute(rg, graphicsCB, computeCB, dm->currentFrame);

    // Transition swapchain image -> PRESENT_SRC_KHR.
    transitionImageLayout(NULL, VK_NULL_HANDLE,
                          gdp->swapchain->p_scImages[dm->currentFrame],
                          gdp->swapchain->currentSurfaceFormat->format,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1,
                          graphicsCB);
    vkEndCommandBuffer(graphicsCB);
    vkEndCommandBuffer(computeCB);

    // Submit both queues using the timeline semaphore pair.
    rgSubmit(rg, graphicsCB, computeCB, sync,
             gdp->imgViewSemaphore[dm->currentFrame],
             gdp->renderEndFlag[dm->currentFrame]);

    // Present.
    VkSemaphore presentWait[] = {gdp->renderEndFlag[dm->currentFrame]};
    VkSwapchainKHR swapChains[]  = {dm->graphicPipeline->swapchain->VkSwapchain};
    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = presentWait,
        .swapchainCount     = 1,
        .pSwapchains        = swapChains,
        .pImageIndices      = &aquireImageIndex,
    };
    VkResult presentResult = vkQueuePresentKHR(*ld->queueManager.presentQueue, &presentInfo);

    dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;
}'''
    story.append(CodeBlock(drawframe_code,
                           filename="src/engine/display/display.c :: drawFrame (condensed)",
                           start_line=314))
    story.append(Spacer(1, 8))
    story.append(PageBreak())

    # =============================================================
    # 4. STAGE-BY-STAGE
    # =============================================================
    story.append(Paragraph("4. Stage-by-Stage Walkthrough",
                            styles['SectionH1']))

    # -------- 4.1 Semaphore wait
    story.append(Paragraph("4.1 Timeline semaphore wait (CPU pacing)",
                            styles['SectionH2']))
    story.append(P(
        "The first thing drawFrame does is bound frames-in-flight. A "
        "monotonic timeline semaphore, <b>sync->graphicsTimeline</b>, is "
        "signaled by the GPU at the end of each submitted frame's work. "
        "If the CPU has submitted <b>maxFramesInFlight</b> frames but "
        "none have completed, we block until the oldest one does. This "
        "prevents unbounded CPU-side work from running ahead of the GPU "
        "and consuming memory indefinitely. A 100ms timeout protects "
        "against a stuck GPU — we bail the frame and try again next "
        "iteration.", styles))
    sem_code = r'''    FrameSync* sync = &ld->frameSync;

    if (sync->graphicsValue >= sync->maxFramesInFlight)
    {
        uint64_t            waitValue = sync->graphicsValue - sync->maxFramesInFlight + 1;
        VkSemaphoreWaitInfo waitInfo  = {
            .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores    = &sync->graphicsTimeline,
            .pValues        = &waitValue,
        };
        VkResult waitResult = vkWaitSemaphores(device, &waitInfo, 100000000ULL); // 100ms
        if (waitResult == VK_TIMEOUT)
            return;
    }'''
    story.append(CodeBlock(sem_code, filename="display.c (semaphore wait)"))
    story.append(P(
        "The wait is a true GPU-driven pacing mechanism: no fence reset, "
        "no busy-loop. Modern Vulkan timeline semaphores replaced the "
        "older binary-fence ring pattern precisely because they compose "
        "cleanly with multi-queue submission. The same semaphore is "
        "signaled from both graphics and compute queues via "
        "<b>rgSubmit</b> at the end of the frame.", styles))

    # -------- 4.2 Deletion flush
    story.append(Paragraph("4.2 Deferred destruction flush",
                            styles['SectionH2']))
    story.append(P(
        "Once the semaphore wait returns, the oldest frame slot is "
        "guaranteed to be GPU-idle. Any resource enqueued for destruction "
        "at that slot is safe to free now. <b>flushDeletionQueue</b> "
        "walks per-type arrays of destroyables (buffers, images, "
        "descriptor pools, pipelines) and calls the appropriate "
        "destroy function under the slot's epoch. "
        "<b>advanceDeletionQueue</b> rotates to the next slot so the "
        "current frame's destroy calls land in a fresh bucket.", styles))
    del_code = r'''    flushDeletionQueue(&ld->deletionQueue, ld->device, ld->allocator);
    advanceDeletionQueue(&ld->deletionQueue);'''
    story.append(CodeBlock(del_code, filename="display.c (deletion flush)"))
    story.append(P(
        "The key correctness invariant: a Vulkan resource must not be "
        "destroyed while any in-flight command buffer still references "
        "it. The deletion queue guarantees this by delaying every "
        "destroy call by <b>MAX_FRAMES_IN_FLIGHT</b> frames. The slot "
        "semaphore wait above is what makes that delay safe.", styles))

    # -------- 4.3 resolveTransforms
    story.append(Paragraph("4.3 Transform hierarchy resolve",
                            styles['SectionH2']))
    story.append(P(
        "The preorder-flattened array means every ancestor appears "
        "before its descendants in memory. One forward sequential pass "
        "over the hot array is enough to resolve world matrices: when "
        "we visit node <i>i</i>, its parent (if any) has already been "
        "resolved in the same loop.", styles))
    story.append(P(
        "The <b>DIRTY</b> bit is sticky: set by mutations and cleared "
        "only after a node has been resolved this frame. A clean node "
        "whose parent is also clean keeps its cached "
        "<b>worldMatrix</b> from the previous frame and pays zero cost "
        "— not even a matrix multiply. This is what drives the "
        "resolveTransforms cost down to ~1 ms for 20K entities at 20% "
        "dirty load. A second short sweep at the end clears the DIRTY "
        "bit on all alive nodes.", styles))
    resolve_code = r'''void resolveTransforms(TransformManager* man)
{
    TransformHot*   pHot   = man->pTransformsHot;
    TransformWorld* pWorld = man->pTransformsWorld;
    TransformCold*  pCold  = man->pTransformsCold;
    uint32_t        count  = man->count;
    uint32_t        live   = 0;

    // Pass 1: preorder walk. Clean nodes skip with one branch and one
    // cache line load per entry (the 12 B hot struct). pWorld is only
    // loaded when a node is dirty.
    for (uint32_t i = 0; i < count; i++)
    {
        TransformHot* hot = &pHot[i];
        hot->passFlags &= ~TRANSFORM_PASS_DISABLED;

        if (hot->lifecycle == TRANSFORM_DESTROYED || hot->lifecycle == TRANSFORM_DISABLED)
        {
            hot->passFlags |= TRANSFORM_PASS_DISABLED;
            continue;
        }

        int           parentDirty = 0;
        TransformHot* parentHot   = NULL;
        uint32_t      parentIdx   = hot->parentIdx;
        if (parentIdx != TRANSFORM_NO_PARENT)
        {
            parentHot = &pHot[parentIdx];
            if (parentHot->passFlags & TRANSFORM_PASS_DISABLED)
            {
                hot->passFlags |= TRANSFORM_PASS_DISABLED;
                continue;
            }
            parentDirty = (parentHot->passFlags & TRANSFORM_PASS_DIRTY) != 0;
        }

        int selfDirty = (hot->passFlags & TRANSFORM_PASS_DIRTY) != 0;
        if (!selfDirty && !parentDirty)
        {
            // Clean: cached worldMatrix from previous frame stays valid.
            live++;
            continue;
        }

        // Dirty branch: rebuild local matrix from TRS then compose.
        TransformCold* cold = &pCold[i];
        mat4 localMatrix;
        transformBuildLocalMatrix(cold, localMatrix);

        if (parentHot != NULL)
            glm_mat4_mul(pWorld[parentIdx].worldMatrix, localMatrix, pWorld[i].worldMatrix);
        else
            glm_mat4_copy(localMatrix, pWorld[i].worldMatrix);

        hot->passFlags |= TRANSFORM_PASS_DIRTY;
        live++;
    }

    // Pass 2: clear DIRTY on alive nodes that got re-resolved.
    for (uint32_t i = 0; i < count; i++)
        if (!(pHot[i].passFlags & TRANSFORM_PASS_DISABLED))
            pHot[i].passFlags &= ~TRANSFORM_PASS_DIRTY;

    man->liveCount = live;
}'''
    story.append(CodeBlock(resolve_code, filename="src/engine/geometry/transformNode.c"))

    # -------- 4.4 Frame globals
    story.append(Paragraph("4.4 Frame globals (view / proj / viewProj)",
                            styles['SectionH2']))
    story.append(P(
        "After resolve, the camera state is frozen: we build one "
        "<b>FrameGlobals</b> struct and memcpy it into the mapped UBO "
        "at binding 9. Every pipeline that needs view/projection reads "
        "from this UBO; invViewProj is precomputed here because the "
        "light fragment shader needs it to reconstruct world-space "
        "position from gl_FragCoord.", styles))
    fg_code = r'''void updateFrameGlobals(FrameGlobals* globals, GraphicSwapchain* dsm, Camera3D camera)
{
    glm_lookat(camera.eye, camera.center, camera.up, globals->view);

    float aspectRatio = dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height;
    glm_perspective(glm_rad(45.0f), aspectRatio, 0.1f, 2000.0f, globals->proj);
    globals->proj[1][1] *= -1; // Vulkan Y-flip

    glm_mat4_mul(globals->proj, globals->view, globals->viewProj);
    glm_mat4_inv(globals->viewProj, globals->invViewProj);

    globals->camera = camera;
}'''
    story.append(CodeBlock(fg_code, filename="src/engine/buffer/uniformBuffer.c"))
    story.append(P(
        "Far plane <b>2000</b> accommodates the 1200-unit-wide bench "
        "scene; the 45-degree vertical FOV matches the hardcoded value "
        "used by the LOD classifier in Section 4.6 for "
        "<b>pixelScale</b>.", styles))

    # -------- 4.5 Frustum cull
    story.append(Paragraph("4.5 Frustum plane extraction + cull",
                            styles['SectionH2']))
    story.append(P(
        "<b>frustumExtractPlanes</b> uses the Gribb/Hartmann trick to "
        "derive six view-frustum planes directly from the viewProj "
        "matrix rows. Vulkan's [0,1] depth range means the near plane "
        "is <i>row2</i> alone, not <i>row3 + row2</i> as in OpenGL. "
        "Planes are normalized so a p-vertex test (pick the AABB corner "
        "most aligned with the plane normal) is a dot product.", styles))
    plane_code = r'''void frustumExtractPlanes(mat4 viewProj, FrustumPlanes* out)
{
    vec4 r0, r1, r2, r3;
    extractRow(viewProj, 0, r0);
    extractRow(viewProj, 1, r1);
    extractRow(viewProj, 2, r2);
    extractRow(viewProj, 3, r3);

    // Left   = row3 + row0
    // Right  = row3 - row0
    // Bottom = row3 + row1
    // Top    = row3 - row1
    // Near   = row2         (Vulkan-specific)
    // Far    = row3 - row2
    for (int i = 0; i < 4; i++) out->planes[0][i] = r3[i] + r0[i];
    for (int i = 0; i < 4; i++) out->planes[1][i] = r3[i] - r0[i];
    for (int i = 0; i < 4; i++) out->planes[2][i] = r3[i] + r1[i];
    for (int i = 0; i < 4; i++) out->planes[3][i] = r3[i] - r1[i];
    for (int i = 0; i < 4; i++) out->planes[4][i] = r2[i];
    for (int i = 0; i < 4; i++) out->planes[5][i] = r3[i] - r2[i];

    for (int i = 0; i < 6; i++) normalizePlane(out->planes[i]);
}'''
    story.append(CodeBlock(plane_code, filename="src/engine/geometry/frustum.c"))

    story.append(P(
        "<b>frustumCullScene</b> walks <b>pTransformsHot</b> in "
        "preorder. When a node is already disabled by resolve "
        "(lifecycle or cascade), or fails the per-entity AABB test, "
        "its entire subtree is skipped. For the lifecycle/cascade case "
        "resolve has already propagated DISABLED; we skip in O(1). For "
        "a frustum miss we must propagate to <b>skipCount</b> "
        "descendants ourselves — still a huge win over testing each.",
        styles))
    cull_code = r'''void frustumCullScene(SCENE* scene, const FrustumPlanes* frustum)
{
    TransformManager* tman = &scene->transformManager;
    uint32_t i = 0;
    while (i < tman->count)
    {
        TransformHot* hot = &tman->pTransformsHot[i];

        // Fast path: already disabled by resolve — bulk-skip the subtree.
        if (hot->passFlags & TRANSFORM_PASS_DISABLED)
        {
            i += 1 + hot->skipCount;
            continue;
        }

        // Cross over to the backing instance for per-instance lifecycle.
        PreorderEntity* entity = &tman->pPreorderEntities[i];
        int alive = 1;
        if ((SceneEntityKind) entity->kind == SCENE_ENTITY_MODEL) {
            Model3D* type = &scene->modelsManager.pModel[entity->typeIdx];
            if (type->alive != MODEL_ALIVE) alive = 0;
            else {
                Model3DInstance* inst = &scene->modelsManager.pDenseModelInstances[entity->denseIdx];
                if (inst->lifecycle != INSTANCE_ALIVE) alive = 0;
            }
        } else { /* SCENE_ENTITY_LIGHT — symmetric check */ }

        if (!alive) {
            hot->passFlags |= TRANSFORM_PASS_DISABLED;
            markSubtreeDisabled(tman->pTransformsHot, i);
            i += 1 + hot->skipCount;
            continue;
        }

        // Actual frustum test against per-entity local AABB.
        if (!frustumTestAABB(frustum, &tman->pLocalAABBs[i], tman->pTransformsWorld[i].worldMatrix))
        {
            hot->passFlags |= TRANSFORM_PASS_DISABLED;
            markSubtreeDisabled(tman->pTransformsHot, i);
            i += 1 + hot->skipCount;
            continue;
        }

        i++;
    }
}'''
    story.append(CodeBlock(cull_code, filename="frustum.c (condensed)"))

    story.append(P(
        "<b>frustumTestAABB</b> is the inner kernel. Arvo's method "
        "transforms the local AABB to world space without instantiating "
        "the 8 corners — one signed multiply/accumulate per axis per "
        "column. Then the p-vertex test picks, per plane, the corner "
        "most aligned with the plane normal and checks the signed "
        "distance in one dot product.", styles))
    test_code = r'''bool frustumTestAABB(const FrustumPlanes* frustum, const AABB* localAABB, mat4 worldMatrix)
{
    vec3 worldMin, worldMax;
    for (int i = 0; i < 3; i++)
    {
        worldMin[i] = worldMatrix[3][i];
        worldMax[i] = worldMatrix[3][i];
        for (int j = 0; j < 3; j++)
        {
            if (worldMatrix[j][i] >= 0.0f) {
                worldMin[i] += worldMatrix[j][i] * localAABB->min[j];
                worldMax[i] += worldMatrix[j][i] * localAABB->max[j];
            } else {
                worldMin[i] += worldMatrix[j][i] * localAABB->max[j];
                worldMax[i] += worldMatrix[j][i] * localAABB->min[j];
            }
        }
    }

    for (int i = 0; i < 6; i++)
    {
        float a = frustum->planes[i][0], b = frustum->planes[i][1];
        float c = frustum->planes[i][2], d = frustum->planes[i][3];
        float px = (a >= 0.0f) ? worldMax[0] : worldMin[0];
        float py = (b >= 0.0f) ? worldMax[1] : worldMin[1];
        float pz = (c >= 0.0f) ? worldMax[2] : worldMin[2];
        if (a * px + b * py + c * pz + d < 0.0f)
            return false;
    }
    return true;
}'''
    story.append(CodeBlock(test_code, filename="frustum.c (Arvo + p-vertex)"))

    # -------- 4.6 Model UBO fill
    story.append(Paragraph("4.6 Model UBO fill with LOD partitioning",
                            styles['SectionH2']))
    story.append(P(
        "This is the largest per-frame CPU function. It combines three "
        "things: LOD classification, compaction of live instances, and "
        "writing mat4 world matrices into the mapped host-visible SSBO. "
        "Two passes per model type: pass 1 classifies every alive + "
        "non-cascade-disabled instance into one of MODEL_LOD_COUNT "
        "buckets based on projected pixel size; pass 2 writes each live "
        "instance's world matrix into its correct slot in the SSBO, "
        "sorted by LOD tier.", styles))
    story.append(P(
        "The draw loop later in the frame (Section 4.9) emits one "
        "<b>vkCmdDrawIndexed</b> call per (type, LOD) combination, "
        "using <b>lodLiveInstanceCount[k]</b> as instanceCount and "
        "<b>instanceOffset[type] + running_lod_offset</b> as "
        "firstInstance. The partitioned SSBO layout makes that a pure "
        "index arithmetic.", styles))
    lod_code = r'''void updateModelUniformBuffersDirect(void* mappedBuffer, SCENE* scene)
{
    ModelInstanceUBO* pInstances         = (ModelInstanceUBO*) mappedBuffer;
    uint32_t          transformationsIdx = 0;

    // Lazy-init the LOD scratch buffer (once per scene).
    if (g_lodScratch == NULL || g_lodScratchCap < scene->modelsManager.modelInstanceCapacity) {
        free(g_lodScratch);
        g_lodScratchCap = scene->modelsManager.modelInstanceCapacity;
        g_lodScratch    = (uint8_t*) malloc(g_lodScratchCap);
    }

    const float fovY       = glm_rad(45.0f);
    const float halfH      = scene->camera.screenSize[1] * 0.5f;
    const float pixelScale = halfH / tanf(fovY * 0.5f);
    vec3 camEye;
    glm_vec3_copy(scene->camera.eye, camEye);

    for (uint32_t modelIdx = 0; modelIdx < scene->modelsManager.modelLen; modelIdx++)
    {
        Model3D* model = &scene->modelsManager.pModel[modelIdx];
        scene->modelsManager.instanceOffset.items[modelIdx] = transformationsIdx;

        if (model->alive != 1) {
            model->liveInstanceCount = 0;
            for (int k = 0; k < MODEL_LOD_COUNT; k++) model->lodLiveInstanceCount[k] = 0;
            continue;
        }

        uint32_t lodCount[MODEL_LOD_COUNT] = {0};
        float    modelRadius               = model->localAABBRadius;

        // Pass 1: classify each live instance into a LOD bucket.
        for (uint32_t instanceIdx = 0; instanceIdx < model->instanceCount; instanceIdx++)
        {
            uint32_t denseIdx = model->denseBaseOffset + instanceIdx;
            Model3DInstance* inst = &scene->modelsManager.pDenseModelInstances[denseIdx];
            if (inst->lifecycle != INSTANCE_ALIVE) { g_lodScratch[denseIdx] = 255; continue; }

            TransformHot*   hot   = &scene->transformManager.pTransformsHot[inst->transformIdx];
            TransformWorld* world = &scene->transformManager.pTransformsWorld[inst->transformIdx];
            if (hot->passFlags & TRANSFORM_PASS_DISABLED) { g_lodScratch[denseIdx] = 255; continue; }

            float wx = world->worldMatrix[3][0] - camEye[0];
            float wy = world->worldMatrix[3][1] - camEye[1];
            float wz = world->worldMatrix[3][2] - camEye[2];
            float dist = sqrtf(wx*wx + wy*wy + wz*wz);

            // Fold in instance world scale so LOD selection scales with the asset.
            float sx = sqrtf(world->worldMatrix[0][0]*world->worldMatrix[0][0] +
                             world->worldMatrix[0][1]*world->worldMatrix[0][1] +
                             world->worldMatrix[0][2]*world->worldMatrix[0][2]);
            float effectiveRadius = modelRadius * sx;
            float pixelSize = (dist > 1e-6f) ? (effectiveRadius * pixelScale / dist) : 1e6f;

            // Pick finest LOD whose threshold is satisfied.
            uint8_t lodIdx = MODEL_LOD_COUNT - 1;
            for (int k = 0; k < MODEL_LOD_COUNT - 1; k++)
                if (pixelSize >= model->lod[k].minPixelSize) { lodIdx = k; break; }

            g_lodScratch[denseIdx] = lodIdx;
            lodCount[lodIdx]++;
        }

        // Prefix-sum per-LOD base offsets within this type's SSBO range.
        uint32_t lodBase[MODEL_LOD_COUNT];
        uint32_t lodCursor[MODEL_LOD_COUNT];
        lodBase[0] = 0;
        for (int k = 1; k < MODEL_LOD_COUNT; k++) lodBase[k] = lodBase[k-1] + lodCount[k-1];
        for (int k = 0; k < MODEL_LOD_COUNT; k++) lodCursor[k] = lodBase[k];

        // Pass 2: scatter world matrices into their LOD-partitioned slots.
        for (uint32_t instanceIdx = 0; instanceIdx < model->instanceCount; instanceIdx++)
        {
            uint32_t denseIdx = model->denseBaseOffset + instanceIdx;
            uint8_t  lod      = g_lodScratch[denseIdx];
            if (lod == 255) continue;

            TransformWorld* world = &scene->transformManager.pTransformsWorld
                [scene->modelsManager.pDenseModelInstances[denseIdx].transformIdx];
            uint32_t ssboIdx = transformationsIdx + lodCursor[lod]++;
            glm_mat4_copy(world->worldMatrix, pInstances[ssboIdx].model);
        }

        uint32_t totalLive = 0;
        for (int k = 0; k < MODEL_LOD_COUNT; k++) {
            model->lodLiveInstanceCount[k] = lodCount[k];
            totalLive += lodCount[k];
        }
        model->liveInstanceCount = totalLive;
        transformationsIdx += totalLive;
    }
    scene->modelsManager.modelInstanceLen = transformationsIdx;
}'''
    story.append(CodeBlock(lod_code, filename="src/engine/buffer/uniformBuffer.c"))

    # -------- 4.7 Light UBO fill (condensed)
    story.append(Paragraph("4.7 Light UBO fill",
                            styles['SectionH2']))
    story.append(P(
        "Mirrors the model fill without the LOD partitioning step "
        "(light proxy geometry is already low-poly). For each light "
        "type, compacts alive instances into the mapped SSBO and "
        "writes the extended <b>LightInstanceUBO</b> (model matrix + "
        "color + position + uniform scale). The uniform scale is "
        "extracted from TransformCold and used by the fragment shader "
        "for distance attenuation.", styles))

    # -------- 4.8 Command recording + staging
    story.append(Paragraph("4.8 Command recording + staging copies",
                            styles['SectionH2']))
    story.append(P(
        "Both UBO fills wrote to host-visible <i>staging</i> buffers. "
        "The graphics command buffer now issues a "
        "<b>vkCmdCopyBuffer</b> from each staging buffer to its "
        "device-local counterpart, followed by a buffer memory barrier "
        "transitioning access from TRANSFER_WRITE to SHADER_READ. "
        "This costs tiny CPU time and moves the upload onto the GPU "
        "timeline where it overlaps with prior-frame work.", styles))
    copy_code = r'''    {
        UniformBufferManager* modelUbo   = gdp->deviceBuffer->pUbo;
        VkBufferCopy          copyRegion = {.srcOffset = 0, .dstOffset = 0,
                                            .size = modelUbo->stagingSize};
        vkCmdCopyBuffer(graphicsCB, modelUbo->stagingBuffers[dm->currentFrame],
                        modelUbo->uniformBuffers[dm->currentFrame], 1, &copyRegion);

        VkBufferMemoryBarrier barrier = {
            .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = modelUbo->uniformBuffers[dm->currentFrame],
            .offset              = 0,
            .size                = modelUbo->stagingSize,
        };
        vkCmdPipelineBarrier(graphicsCB, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, NULL,
                             1, &barrier, 0, NULL);
    }'''
    story.append(CodeBlock(copy_code, filename="display.c (model SSBO staging copy)"))

    # -------- 4.9 Render graph execute
    story.append(Paragraph("4.9 Render graph execute + pass callbacks",
                            styles['SectionH2']))
    story.append(P(
        "<b>rgExecute</b> walks the compiled execution order. Each "
        "<b>RGPass</b> has associated input/output resources, "
        "auto-inserted acquire/release barriers, and a user callback. "
        "For this engine the pass set is (in order): "
        "depthPrepass, models, lights, particles on the graphics "
        "queue, and particleCompute on the compute queue.", styles))
    story.append(P(
        "The depth prepass and the model pass are coupled by a critical "
        "invariant: they must issue <i>identical</i> draw calls "
        "(same LOD selection per instance, same type ordering) or the "
        "LESS_OR_EQUAL depth test in the color pass z-fights. The "
        "enforcement is indirect: both callbacks read the same "
        "<b>lodLiveInstanceCount[]</b> on each Model3D, and both loop "
        "over types in the same order. A comment block at both sites "
        "records this coupling for future edits.", styles))
    depth_code = r'''void executeDepthPrepass(VkCommandBuffer cmd, const RGPass* pass, void* userData)
{
    DepthPrepassData*         data     = (DepthPrepassData*) userData;
    GraphicPipelineToSurface* depthGdp = data->depthPipeline;
    GraphicPipelineToSurface* modelGdp = data->modelPipeline;
    uint32_t                  frame    = data->dm->currentFrame;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *depthGdp->pipeline);

    VkViewport viewport = { 0, 0,
        (float) modelGdp->swapchain->currentExtent2D.width,
        (float) modelGdp->swapchain->currentExtent2D.height, 0.0f, 1.0f };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor = { {0, 0}, modelGdp->swapchain->currentExtent2D };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *depthGdp->pipelineLayout,
                            0, 1, &modelGdp->descriptorSet[frame], 0, VK_NULL_HANDLE);

    VkDeviceSize offsets[]   = {0};
    VkBuffer     vBuf[]      = {*modelGdp->deviceBuffer->buffer};
    vkCmdBindVertexBuffers(cmd, 0, 1, vBuf, offsets);
    vkCmdBindIndexBuffer(cmd, *modelGdp->deviceBuffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // LOD draw loop — MUST be byte-identical to executeModelPass.
    const SCENE* scene = modelGdp->scene;
    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        Model3D* model = &scene->modelsManager.pModel[i];
        if (model->alive != 1) continue;
        uint32_t typeBase  = scene->modelsManager.instanceOffset.items[i];
        uint32_t lodOffset = 0;
        for (int k = 0; k < MODEL_LOD_COUNT; k++)
        {
            uint32_t count = model->lodLiveInstanceCount[k];
            if (count == 0) continue;
            vkCmdDrawIndexed(cmd,
                             model->lod[k].indexCount,
                             count,
                             model->lod[k].indexOffset,
                             model->lod[k].vertexOffset,
                             typeBase + lodOffset);
            lodOffset += count;
        }
    }
}'''
    story.append(CodeBlock(depth_code, filename="src/engine/rendergraph/passCallbacks.c"))

    story.append(P(
        "The model pass is structurally identical to the depth prepass "
        "above, differing only in the pipeline bound and that it "
        "targets the color attachment instead of depth-only. The "
        "shared descriptor set (binding 0 = model SSBO, binding 9 = "
        "FrameGlobals UBO) means both passes read from the same UBO "
        "writes performed earlier in drawFrame.", styles))

    # -------- 4.10 Submit + present
    story.append(Paragraph("4.10 Submit + present",
                            styles['SectionH2']))
    story.append(P(
        "<b>rgSubmit</b> submits the graphics and compute command "
        "buffers on their respective queues, signaling "
        "<b>graphicsTimeline</b> at the end so the CPU wait in the "
        "next frame's stage 4.1 completes when this frame is GPU-done. "
        "Compute-to-graphics handoffs inside the frame use the same "
        "timeline semaphore across queues — the graphics queue waits "
        "on the compute queue's timeline value before consuming any "
        "particle-compute output.", styles))
    story.append(P(
        "<b>vkQueuePresentKHR</b> waits on the "
        "<b>renderEndFlag</b> binary semaphore signaled at the end of "
        "graphics submission, then queues the swapchain image for "
        "presentation. On SUBOPTIMAL or OUT_OF_DATE it triggers a "
        "swapchain recreation and bails.", styles))
    story.append(PageBreak())

    # =============================================================
    # 5. Performance envelope
    # =============================================================
    story.append(Paragraph("5. Performance Envelope", styles['SectionH1']))
    story.append(P(
        "Measured on an RTX 3060 at 900 MHz locked, 1280x720 viewport, "
        "320-frame bench, 20,100 entities (20,000 model instances + "
        "100 light roots) arranged as a 10x10 preorder group grid "
        "spanning +/- 540 units, plus 1,000,000 GPU-simulated particles, "
        "20% transform dirty fraction, LOD thresholds 20 / 15 / 0 pixels.",
        styles))
    perf = [
        ["Scenario", "resolve", "cull", "totalCpu (med)", "LOD0 / LOD1 / LOD2 share"],
        ["1. top-down (20,000 visible)",  "~1.0 ms",  "~2.0 ms",  "~5.4 ms", "0% / 0% / 100%"],
        ["2. low edge grazing",            "~1.0 ms",  "~1.9 ms",  "~5.1 ms", "0% / 0% / 100%"],
        ["3. corner close-up (1,791)",     "~0.96 ms", "~0.70 ms", "~7.4 ms", "32% / 26% / 42%"],
        ["4. facing away (0 visible)",     "~0.99 ms", "~0.07 ms", "~7.5 ms", "--"],
        ["5. inside grid, +X view",        "~0.99 ms", "~0.63 ms", "~8.0 ms", "10% / 11% / 79%"],
    ]
    t = Table(perf, colWidths=[CONTENT_W * 0.32, CONTENT_W * 0.13,
                                CONTENT_W * 0.13, CONTENT_W * 0.17,
                                CONTENT_W * 0.25])
    t.setStyle(TableStyle([
        ('FONT',      (0, 0), (-1, 0), 'Helvetica-Bold', 8.5),
        ('FONT',      (0, 1), (-1, -1), 'Helvetica', 8),
        ('BACKGROUND',(0, 0), (-1, 0), COLORS['accent2']),
        ('TEXTCOLOR', (0, 0), (-1, 0), COLORS['white']),
        ('GRID',      (0, 0), (-1, -1), 0.5, COLORS['border_light']),
        ('VALIGN',    (0, 0), (-1, -1), 'MIDDLE'),
        ('LEFTPADDING',  (0, 0), (-1, -1), 4),
        ('RIGHTPADDING', (0, 0), (-1, -1), 4),
        ('TOPPADDING',   (0, 0), (-1, -1), 3),
        ('BOTTOMPADDING',(0, 0), (-1, -1), 3),
        ('ROWBACKGROUNDS',(0, 1), (-1, -1), [COLORS['bg_struct'], COLORS['bg_struct_alt']]),
    ]))
    story.append(t)
    story.append(Paragraph(
        "Table 5.1 - Per-scenario CPU breakdown and LOD distribution.",
        styles['Caption']))

    story.append(Spacer(1, 6))
    story.append(P(
        "What this says in one sentence: transforming and culling "
        "20,100 entities and filling their LOD-partitioned SSBOs takes "
        "the CPU about 3 milliseconds, regardless of scenario. The rest "
        "of the <i>totalCpu</i> time is GPU fence wait, which is "
        "dominated by the 1,000,000-particle compute pass and the "
        "render-graph pass cost.", styles))

    return story


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    out_path = "/home/jamh/Desktop/humo2/doc/humo2_frame_lifecycle.pdf"

    styles = build_styles()
    story = build_story(styles)

    doc = SimpleDocTemplate(
        out_path,
        pagesize=A4,
        leftMargin=MARGIN_LEFT,
        rightMargin=MARGIN_RIGHT,
        topMargin=MARGIN_TOP,
        bottomMargin=MARGIN_BOTTOM,
        title="humo2 Frame Lifecycle Reference",
        author="Generated by renderer-doc-skill",
    )

    doc.build(story, onFirstPage=title_page_bg, onLaterPages=footer)
    size = os.path.getsize(out_path)
    print(f"Wrote {out_path}  ({size/1024:.1f} KB)")

if __name__ == '__main__':
    main()
