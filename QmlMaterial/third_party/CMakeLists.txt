set(md_dir material-color/cpp)
add_library(
  material_color STATIC
  ${md_dir}/dynamiccolor/material_dynamic_colors.cc
  ${md_dir}/dynamiccolor/dynamic_color.cc
  ${md_dir}/dynamiccolor/dynamic_scheme.cc
  ${md_dir}/scheme/scheme_content.cc
  ${md_dir}/scheme/scheme_neutral.cc
  ${md_dir}/scheme/scheme_expressive.cc
  ${md_dir}/scheme/scheme_fidelity.cc
  ${md_dir}/scheme/scheme_fruit_salad.cc
  ${md_dir}/scheme/scheme_monochrome.cc
  ${md_dir}/scheme/scheme_rainbow.cc
  ${md_dir}/scheme/scheme_tonal_spot.cc
  ${md_dir}/scheme/scheme_vibrant.cc
  ${md_dir}/palettes/tones.cc
  ${md_dir}/utils/utils.cc
  ${md_dir}/temperature/temperature_cache.cc
  ${md_dir}/quantize/lab.cc
  ${md_dir}/quantize/celebi.cc
  ${md_dir}/quantize/wsmeans.cc
  ${md_dir}/quantize/wu.cc
  ${md_dir}/score/score.cc
  ${md_dir}/dislike/dislike.cc
  ${md_dir}/contrast/contrast.cc
  ${md_dir}/cam/cam.cc
  ${md_dir}/cam/hct.cc
  ${md_dir}/cam/hct_solver.cc
  ${md_dir}/cam/viewing_conditions.cc
  ${md_dir}/blend/blend.cc)

target_compile_features(material_color PRIVATE cxx_std_20)
target_compile_options(material_color PRIVATE -w)
target_include_directories(material_color PUBLIC material-color)