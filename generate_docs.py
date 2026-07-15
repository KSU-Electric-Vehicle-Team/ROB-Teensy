import os
import subprocess

Import("env") # type:ignore

def build_doxygen(source, target, env):
    print("Generating Doxygen documentation...")
    if os.path.exists("Doxyfile"):
        # Executes 'doxygen Doxyfile' via system terminal
        subprocess.run(["doxygen", "Doxyfile"])
    else:
        print("Warning: Doxyfile not found.")

# Trigger documentation build after compiling the project
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", build_doxygen) # type:ignore