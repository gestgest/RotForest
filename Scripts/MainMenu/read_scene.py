# MainMenu 맵 읽기 전용 (아무것도 바꾸지 않음)
import unreal


def prop(obj, name, default="?"):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def hexc(c):
    return "#%02X%02X%02X" % (c.r, c.g, c.b)


def lin(c):
    return "(%.4f, %.4f, %.4f)" % (c.r, c.g, c.b)


world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
print("=== WORLD: %s / ACTORS: %d ===" % (world.get_name(), len(actors)))

for a in sorted(actors, key=lambda x: x.get_actor_label()):
    l = a.get_actor_location()
    r = a.get_actor_rotation()
    s = a.get_actor_scale3d()
    print("[%s] (%s)  Loc(%.1f, %.1f, %.1f)  Rot(P%.1f Y%.1f R%.1f)  Scale(%.2f, %.2f, %.2f)" % (
        a.get_actor_label(), a.get_class().get_name(), l.x, l.y, l.z, r.pitch, r.yaw, r.roll, s.x, s.y, s.z))

    for cam in a.get_components_by_class(unreal.CineCameraComponent):
        print("    Camera: FocalLength=%.1f" % cam.current_focal_length)

    for lc in a.get_components_by_class(unreal.LightComponent):
        extra = ""
        if isinstance(lc, unreal.PointLightComponent):
            extra = "  AttenRadius=%.0f  Units=%s" % (lc.attenuation_radius, prop(lc, "intensity_units"))
        print("    Light(%s): Intensity=%.3f  Color=%s%s" % (
            lc.get_class().get_name(), lc.intensity, hexc(lc.light_color), extra))

    for sk in a.get_components_by_class(unreal.SkyLightComponent):
        print("    SkyLight: Intensity=%.3f" % sk.intensity)

    for f in a.get_components_by_class(unreal.ExponentialHeightFogComponent):
        print("    Fog: Density=%.4f  Inscattering=%s  Volumetric=%s" % (
            f.fog_density, lin(prop(f, "fog_inscattering_luminance")), prop(f, "enable_volumetric_fog")))

    if isinstance(a, unreal.PostProcessVolume):
        st = a.get_editor_property("settings")
        print("    PPV: Unbound=%s  MinEV100=%.2f(ov=%s)  MaxEV100=%.2f(ov=%s)  Vignette=%.2f(ov=%s)  Bloom=%.2f(ov=%s)" % (
            a.get_editor_property("unbound"),
            st.get_editor_property("auto_exposure_min_brightness"), st.get_editor_property("override_auto_exposure_min_brightness"),
            st.get_editor_property("auto_exposure_max_brightness"), st.get_editor_property("override_auto_exposure_max_brightness"),
            st.get_editor_property("vignette_intensity"), st.get_editor_property("override_vignette_intensity"),
            st.get_editor_property("bloom_intensity"), st.get_editor_property("override_bloom_intensity")))

print("=== DONE (읽기만 함) ===")
