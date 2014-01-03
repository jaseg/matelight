
# Hard timeout in seconds after which (approximately) the rendering of a single item will be cut off
RENDERER_TIMEOUT = 20.0
# How long to show an image by default
DEFAULT_IMAGE_DURATION = 10.0
# Default scrolling speed in pixels/second
DEFAULT_SCROLL_SPEED = 4
# Pixels to leave blank between two letters
LETTER_SPACING = 0

# Display geometry
# ┌─────────┐ ┌───┬───┬  ⋯  ┬───┬───┐
# │1 o o o 5│ │ 1 │   │     │   │ 8│
# │6 o o o o│ ├───┼───┼  ⋯  ┼───┼───┤
# │o o o o o│ │ 9 │   │     │   │16 │
# │o o o o20│ ├───┼───┼  ⋯  ┼───┼───┤
# └─────────┘ │17 │   │     │   │24 │
#             ├───┼───┼  ⋯  ┼───┼───┤
#             │25 │   │     │   │32 │
#             └───┴───┴  ⋯  ┴───┴───┘

CRATE_WIDTH = 5
CRATE_HEIGHT = 4
CRATES_X = 8
CRATES_Y = 4

# Computed values
DISPLAY_WIDTH = CRATES_X * CRATE_WIDTH
DISPLAY_HEIGHT = CRATES_Y * CRATE_HEIGHT
FRAME_SIZE = DISPLAY_WIDTH*DISPLAY_HEIGHT*3

