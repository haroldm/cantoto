Import("env")

env.AddCustomTarget(
    name="Reset",
    dependencies=None,
    actions=['openocd -f utils/tigard_swd.cfg -c "init; reset run; exit"'],
    description="Run OpenOCD reset command"
)