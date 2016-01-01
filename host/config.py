
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

# Physical display dimensions
crate_width = 5
crate_height = 4
crates_x = 8
crates_y = 4

# Computed values
display_width = crates_x * crate_width
display_height = crates_y * crate_height
crate_size = crate_width*crate_height
frame_size = display_width*display_height

# Display gamma factor
gamma = 2.5

# Brightness of the display. 0 to 1.0
brightness = 1.0

# Frame timeout for UDP clients
udp_timeout = 3.0

# Interval for rotation of multiple concurrent UDP clients
udp_switch_interval = 30.0

# Listening addr/port for UDP and TCP servers
udp_addr = tcp_addr = ''
udp_port = tcp_port = 1337

# Forward addr/port
crap_fw_addr, crap_fw_port = '127.0.0.1', 1338

# USB Serial number of matelight to control as byte string (None for first matelight connected)
ml_usb_serial_match = None

