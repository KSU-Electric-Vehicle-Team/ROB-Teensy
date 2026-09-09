import os
import subprocess

Import("env") # type:ignore

def build_doxygen(source, target, env):
    print("Generating Doxygen documentation...")
    if os.path.exists("Doxyfile"):
        subprocess.run(["doxygen", "Doxyfile"], check=True)
    else:
        print("Warning: Doxyfile not found.")

env.AddCustomTarget(
    name="docs",
    dependencies=None,
    actions=[build_doxygen],
    title="Documentation",
    description="Generate Doxygen documentation",
) # type:ignore
