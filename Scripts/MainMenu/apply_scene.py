# MainMenu 맵 적용 스크립트 (맵 저장은 하지 않음. 되돌리기: Ctrl+Z 한 번)
import math
import random
import unreal

TREE_DEAD = "/Game/Necropolis/Foliage/Meshes/SM_tree_dead"
PINE = "/Game/Necropolis/Foliage/Meshes/SM_pine"
CAMPFIRE = unreal.Vector(0, 150, 0)

changes = []


def log(what, before, after):
    changes.append((what, before, after))


def warn(msg):
    unreal.log_warning("[MainMenu] " + msg)


def srgb_to_linear(hex_str):
    h = hex_str.lstrip("#")
    out = []
    for i in (0, 2, 4):
        v = int(h[i:i + 2], 16) / 255.0
        out.append(v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4)
    return unreal.LinearColor(out[0], out[1], out[2], 1.0)


def lin_str(c):
    return "(%.4f, %.4f, %.4f)" % (c.r, c.g, c.b)


eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def labels():
    return {a.get_actor_label(): a for a in eas.get_all_level_actors()}


with unreal.ScopedEditorTransaction("MainMenu Scene") as trans:
    by_label = labels()

    # [1] 울타리 삭제 (라벨에 fence 포함, 대소문자 무시)
    deleted = []
    for label, a in list(by_label.items()):
        if "fence" in label.lower():
            deleted.append(label)
            eas.destroy_actor(a)
    if deleted:
        log("울타리 삭제", "-", ", ".join(deleted))
        print("삭제한 울타리:", deleted)
    else:
        warn("라벨에 Fence가 들어간 액터 없음")
    by_label = labels()

    # [2] 카메라 Pitch -3
    cam = by_label.get("Cam_MainMenu")
    if cam:
        cam.modify()
        r = cam.get_actor_rotation()
        cam.set_actor_rotation(unreal.Rotator(roll=r.roll, pitch=-3.0, yaw=r.yaw), False)
        log("Cam_MainMenu Pitch", "%.1f" % r.pitch, "-3.0")
    else:
        warn("Cam_MainMenu 없음")

    # [3] 안개
    fog_actor = by_label.get("ExponentialHeightFog")
    if fog_actor:
        f = fog_actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
        f.modify()
        old_d = f.get_editor_property("fog_density")
        old_c = f.get_editor_property("fog_inscattering_luminance")
        old_v = f.get_editor_property("enable_volumetric_fog")
        new_c = srgb_to_linear("#1C2A1F")
        f.set_editor_property("fog_density", 0.06)
        f.set_editor_property("fog_inscattering_luminance", new_c)
        f.set_editor_property("enable_volumetric_fog", True)
        log("Fog Density", "%.4f" % old_d, "0.0600")
        log("Fog Inscattering", lin_str(old_c), lin_str(new_c) + " (#1C2A1F)")
        log("Fog Volumetric", str(old_v), "True")
    else:
        warn("ExponentialHeightFog 없음")

    # [4] SkyLight
    sky = by_label.get("SkyLight")
    if sky:
        sc = sky.get_component_by_class(unreal.SkyLightComponent)
        sc.modify()
        old = sc.get_editor_property("intensity")
        sc.set_editor_property("intensity", 0.15)
        log("SkyLight Intensity", "%.3f" % old, "0.150")
    else:
        warn("SkyLight 없음")

    # [5] PostProcessVolume
    ppv = by_label.get("PostProcessVolume")
    if ppv:
        ppv.modify()
        old_unbound = ppv.get_editor_property("unbound")
        ppv.set_editor_property("unbound", True)
        log("PPV Infinite Extent", str(old_unbound), "True")

        st = ppv.get_editor_property("settings")
        ov_min = st.get_editor_property("override_auto_exposure_min_brightness")
        ov_max = st.get_editor_property("override_auto_exposure_max_brightness")
        cur_min = st.get_editor_property("auto_exposure_min_brightness")
        cur_max = st.get_editor_property("auto_exposure_max_brightness")
        if ov_min:
            ev = cur_min + 1.0
        elif ov_max:
            ev = cur_max + 1.0
        else:
            ev = 2.0
        st.set_editor_property("override_auto_exposure_min_brightness", True)
        st.set_editor_property("override_auto_exposure_max_brightness", True)
        st.set_editor_property("auto_exposure_min_brightness", ev)
        st.set_editor_property("auto_exposure_max_brightness", ev)
        log("PPV EV100 Min", "%.2f(ov=%s)" % (cur_min, ov_min), "%.2f" % ev)
        log("PPV EV100 Max", "%.2f(ov=%s)" % (cur_max, ov_max), "%.2f" % ev)

        old_v = st.get_editor_property("vignette_intensity")
        old_b = st.get_editor_property("bloom_intensity")
        st.set_editor_property("override_vignette_intensity", True)
        st.set_editor_property("vignette_intensity", 0.8)
        st.set_editor_property("override_bloom_intensity", True)
        st.set_editor_property("bloom_intensity", 0.5)
        log("PPV Vignette", "%.2f" % old_v, "0.80")
        log("PPV Bloom", "%.2f" % old_b, "0.50")
        ppv.set_editor_property("settings", st)
    else:
        warn("PostProcessVolume 없음")

    # [6] 모닥불 Point Light
    pl = by_label.get("PL_Campfire")
    created = False
    if not pl:
        pl = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(0, 150, 60))
        pl.set_actor_label("PL_Campfire")
        created = True
    if pl:
        pl.modify()
        lc = pl.get_component_by_class(unreal.PointLightComponent)
        lc.modify()
        old_i = lc.get_editor_property("intensity")
        pl.set_actor_location(unreal.Vector(0, 150, 60), False, True)
        lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
        lc.set_editor_property("intensity", 20.0)
        lc.set_editor_property("light_color", unreal.Color(r=255, g=138, b=61, a=255))
        lc.set_editor_property("attenuation_radius", 800.0)
        log("PL_Campfire", "새로 생성" if created else "Intensity %.2f" % old_i,
            "(0,150,60) #FF8A3D 20cd Atten 800")
    else:
        warn("PL_Campfire 생성 실패")

    # [7] 숲
    by_label = labels()
    if any(l.startswith("Forest_") for l in by_label):
        print("Forest_ 액터가 이미 있어서 숲은 다시 만들지 않음")
    else:
        m_dead = unreal.load_asset(TREE_DEAD)
        m_pine = unreal.load_asset(PINE)
        if not m_dead or not m_pine:
            warn("나무 메시를 못 불러옴: %s / %s" % (TREE_DEAD, PINE))
        else:
            placed = []  # (x, y)

            def spawn(label, mesh, x, y, yaw, scale):
                a = eas.spawn_actor_from_class(
                    unreal.StaticMeshActor, unreal.Vector(x, y, 0), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
                a.set_actor_label(label)
                a.set_folder_path("MenuForest")
                a.get_editor_property("static_mesh_component").set_static_mesh(mesh)
                a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
                placed.append((x, y))
                return a

            # 액자 나무
            spawn("Forest_Frame_01", m_dead, -850, -380, 0, 1.6)
            spawn("Forest_Frame_02", m_dead, -800, 420, 0, 1.8)

            # 뒤쪽 숲 18그루
            random.seed(7)
            count, attempts = 0, 0
            while count < 18 and attempts < 5000:
                attempts += 1
                x = random.uniform(800, 3000)
                y = random.uniform(-2000, 2000)
                mesh = random.choice([m_dead, m_pine])
                yaw = random.uniform(0, 360)
                scale = random.uniform(0.8, 1.8)
                if math.hypot(x - CAMPFIRE.x, y - CAMPFIRE.y) < 300:
                    continue
                if any(math.hypot(x - px, y - py) < 300 for px, py in placed):
                    continue
                count += 1
                spawn("Forest_%02d" % count, mesh, x, y, yaw, scale)
            log("숲 나무", "0", "액자 2 + 뒤쪽 %d그루" % count)
            if count < 18:
                warn("간격 조건 때문에 %d그루만 배치됨" % count)

print("=== 변경 내역 (전 -> 후) ===")
for what, before, after in changes:
    print("%-22s %s  ->  %s" % (what, before, after))
print("=== 완료. 맵은 저장하지 않았음. 되돌리기: Ctrl+Z ===")
