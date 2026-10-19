# Sun medallion source

Source: the image supplied by the user as `photo_2026-10-19 21.41.45.jpeg`.

[SunMedallion.png](SunMedallion.png) is a transparent cutout made with the built-in imagegen tool.
Its opaque octagonal rim is retained; the black background and circular pedestal
are removed. UE imports it as `/Game/UI/T_SunMedallion` (sRGB UI RGBA, no mipmaps,
clamped edges, no streaming). `Scripts/import_hud_medallion.py` reproduces the import.

Exact edit prompt (`transparent_background=true`):

> Use case: background-extraction. Edit target: the attached photograph of an octagonal golden weathered shield with a red sun and a yellow human face on a circular display pedestal, on black. Extract ONLY the octagonal shield/medallion as a UI texture. Remove the entire black background, the entire round pedestal/base beneath it, and any shadow beyond the octagonal silhouette. Preserve the existing shield image, eight-sided outer stone/metal rim, the sun's face, red rays, weathered golden stone texture and original colors as closely as possible. Do not redesign, stylize, redraw or add objects. Straight-on view matching original. Center the complete octagonal medallion on a square PNG canvas, let the medallion fill 96 percent of the canvas height and width with a tiny even transparent margin. All pixels outside the octagonal silhouette must be genuinely transparent alpha, including the bottom corners that previously overlapped the pedestal. No checkerboard painted into the image, no added frame or lettering.
