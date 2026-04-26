# type: ignore
# noqa
import subprocess
import sys
import os

try:
    Import("env")  # type: ignore # noqa
except:
    pass

def start_logger(source, target, env):
    print("Starting NivaasIQ logger...")
    python = sys.executable
    logger_path = os.path.join(os.getcwd(), "backend", "logger.py")
    subprocess.Popen(
        [python, logger_path],
        creationflags=subprocess.CREATE_NEW_CONSOLE
    )

try:
    env.AddPostAction("upload", start_logger)  # type: ignore # noqa
except:
    pass