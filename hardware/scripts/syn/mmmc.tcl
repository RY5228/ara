
# -------------------------------------------------------------
# Set the SDC FILE
# -------------------------------------------------------------
create_constraint_mode -name common -sdc_files $env(SYN_SCRIPTS_DIR)/constraint.sdc

# -------------------------------------------------------------
# Set the lib
# -------------------------------------------------------------
create_library_set -name setup_set -timing [list $env(SETUP_LIB)]
create_library_set -name hold_set -timing [list $env(HOLD_LIB)]

# -------------------------------------------------------------
# Create timing condition
# -------------------------------------------------------------
create_timing_condition -name setup_cond -library_sets [list setup_set]
create_timing_condition -name hold_cond -library_sets [list hold_set]

# -------------------------------------------------------------
# Create RC corner
# -------------------------------------------------------------
create_rc_corner -name rc_corner 

# -------------------------------------------------------------
# Create the delay corner
# -------------------------------------------------------------
create_delay_corner -name setup_delay -timing_condition setup_cond -rc_corner rc_corner
create_delay_corner -name hold_delay -timing_condition hold_cond -rc_corner rc_corner

# -------------------------------------------------------------
# Create the analysis view
# -------------------------------------------------------------
create_analysis_view -name setup_view -delay_corner setup_delay -constraint_mode common
create_analysis_view -name hold_view -delay_corner hold_delay -constraint_mode common

# -------------------------------------------------------------
# Set the analysis view for setup & hold
# -------------------------------------------------------------
set_analysis_view -setup { setup_view } -hold { hold_view }
