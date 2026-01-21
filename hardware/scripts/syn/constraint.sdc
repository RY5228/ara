# current_design $env(TOP_MODULE)

set_units -capacitance 1.0fF
set_units -time 1.0ps

set clk_name $env(CLK_NAME)
set clk_port_name $env(CLK_PORT_NAME)
set clk_period_ps $env(CLK_PERIOD_PS)
set input_delay_ps $env(INPUT_DELAY_PS)
set output_delay_ps $env(OUTPUT_DELAY_PS)

create_clock -period ${clk_period_ps} -name $clk_name [get_ports ${clk_port_name}]
set_input_delay $input_delay_ps -clock $clk_name [all_inputs -no_clock]
set_output_delay $output_delay_ps -clock $clk_name [all_outputs]
set_clock_groups -asynchronous  -group ${clk_name}

# -------------------------------------------------------------
# Set global constraints
# -------------------------------------------------------------
