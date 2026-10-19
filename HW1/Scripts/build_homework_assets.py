"""Author and save the real Blueprint/Input assets using the editor module."""
from pathlib import Path
import unreal

if not unreal.MechanicsEditorLibrary.build_homework_assets():
    raise RuntimeError("Homework asset setup failed; see the Unreal log.")
output = Path(unreal.Paths.project_saved_dir()) / "Verification" / "assets.json"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(unreal.MechanicsEditorLibrary.inspect_project(), encoding="utf-8")
unreal.log("HW1: all homework assets built and saved successfully")
