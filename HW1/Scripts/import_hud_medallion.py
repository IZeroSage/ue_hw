"""Import the transparent medallion as a UI texture with preserved sRGB and alpha."""
from pathlib import Path
import unreal

source = Path(unreal.Paths.project_dir()) / "SourceArt/UI/SunMedallion.png"
task = unreal.AssetImportTask()
task.filename = str(source.resolve())
task.destination_path = "/Game/UI"
task.destination_name = "T_SunMedallion"
task.automated = True
task.replace_existing = True
task.save = False
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset("/Game/UI/T_SunMedallion")
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError("Medallion texture import failed")
texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property("srgb", True)
texture.set_editor_property("never_stream", True)
texture.set_editor_property("max_texture_size", 512)
texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
    raise RuntimeError("Could not save medallion texture")
unreal.log("HW1: imported /Game/UI/T_SunMedallion with alpha")
