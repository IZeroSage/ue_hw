"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>."""
from pathlib import Path
import unreal

output = Path(unreal.Paths.project_saved_dir()) / "Verification" / "assets.json"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(unreal.MechanicsEditorLibrary.inspect_project(), encoding="utf-8")
unreal.log(f"Homework asset report: {output}")
