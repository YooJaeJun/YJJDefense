"""생성한 PNG·WAV를 언리얼 에셋으로 만든다 (에디터 Python, 커맨드릿에서 실행).

만드는 것:
  /Game/YJJDefense/Textures/T_*   그림 (Canvas로 직접 그리므로 스프라이트 에셋은 없다)
  /Game/YJJDefense/Audio/*        효과음·배경음
  /Game/YJJDefense/Maps/L_Battle  빈 맵 (게임모드와 HUD가 전부 그린다)

여러 번 실행해도 같은 결과가 나오도록 있으면 갱신, 없으면 생성한다.
실행은 Tools/Art/BuildArt.ps1 참고.
"""

import os

import unreal

ROOT = "/Game/YJJDefense"
TEXTURE_DIR = ROOT + "/Textures"
AUDIO_DIR = ROOT + "/Audio"
MAP_DIR = ROOT + "/Maps"

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
ART_DIR = os.path.join(PROJECT_DIR, "Tools", "Art", "Generated")
SOUND_DIR = os.path.join(PROJECT_DIR, "Tools", "Audio", "Generated")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
assets = unreal.EditorAssetLibrary


def log(message):
    unreal.log("[YJJArt] " + message)


def import_file(path, destination, name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    asset_tools.import_asset_tasks([task])
    asset = assets.load_asset(destination + "/" + name)
    if asset is None:
        raise RuntimeError("Failed to import " + path)
    return asset


def import_textures():
    wanted = set()
    for file_name in sorted(os.listdir(ART_DIR)):
        if not file_name.endswith(".png") or file_name.startswith("_"):
            continue
        name = file_name[:-4]
        wanted.add(name)
        texture = import_file(os.path.join(ART_DIR, file_name), TEXTURE_DIR, name)

        # 매끈한 선 그림: 압축 없이, 밉맵 없이, 부드럽게 늘린다
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
        texture.set_editor_property("never_stream", True)
        texture.set_editor_property("srgb", True)
        log("texture " + name)

    # 생성기에서 빠진 그림은 지운다
    for path in assets.list_assets(TEXTURE_DIR, recursive=False):
        name = path.split("/")[-1].split(".")[0]
        if name not in wanted:
            assets.delete_asset(path.split(".")[0])
            log("deleted " + name)


def import_sounds():
    if not os.path.isdir(SOUND_DIR):
        return
    for file_name in sorted(os.listdir(SOUND_DIR)):
        if not file_name.endswith(".wav"):
            continue
        name = file_name[:-4]
        sound = import_file(os.path.join(SOUND_DIR, file_name), AUDIO_DIR, name)
        sound.set_editor_property("looping", name.startswith("BGM"))
        log("sound " + name)


def make_map():
    path = MAP_DIR + "/L_Battle"
    if assets.does_asset_exist(path):
        log("map exists " + path)
        return
    if asset_tools.create_asset("L_Battle", MAP_DIR, unreal.World, unreal.WorldFactory()) is None:
        raise RuntimeError("Failed to create " + path)
    log("map " + path)


def main():
    import_textures()
    import_sounds()
    make_map()
    assets.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
    log("done")


main()
