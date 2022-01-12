project(scwx-data)

find_package(Boost)

set(HDR_COMMON include/scwx/common/characters.hpp
               include/scwx/common/color_table.hpp
               include/scwx/common/constants.hpp
               include/scwx/common/products.hpp
               include/scwx/common/types.hpp
               include/scwx/common/vcp.hpp)
set(SRC_COMMON source/scwx/common/color_table.cpp
               source/scwx/common/products.cpp
               source/scwx/common/vcp.cpp)
set(HDR_UTIL include/scwx/util/iterator.hpp
             include/scwx/util/rangebuf.hpp
             include/scwx/util/streams.hpp
             include/scwx/util/threads.hpp
             include/scwx/util/time.hpp
             include/scwx/util/vectorbuf.hpp)
set(SRC_UTIL source/scwx/util/rangebuf.cpp
             source/scwx/util/streams.cpp
             source/scwx/util/time.cpp
             source/scwx/util/vectorbuf.cpp)
set(HDR_WSR88D include/scwx/wsr88d/ar2v_file.hpp
               include/scwx/wsr88d/level3_file.hpp
               include/scwx/wsr88d/message.hpp)
set(SRC_WSR88D source/scwx/wsr88d/ar2v_file.cpp
               source/scwx/wsr88d/level3_file.cpp
               source/scwx/wsr88d/message.cpp)
set(HDR_WSR88D_RDA include/scwx/wsr88d/rda/clutter_filter_map.hpp
                   include/scwx/wsr88d/rda/digital_radar_data.hpp
                   include/scwx/wsr88d/rda/level2_message.hpp
                   include/scwx/wsr88d/rda/level2_message_factory.hpp
                   include/scwx/wsr88d/rda/level2_message_header.hpp
                   include/scwx/wsr88d/rda/performance_maintenance_data.hpp
                   include/scwx/wsr88d/rda/rda_adaptation_data.hpp
                   include/scwx/wsr88d/rda/rda_status_data.hpp
                   include/scwx/wsr88d/rda/types.hpp
                   include/scwx/wsr88d/rda/volume_coverage_pattern_data.hpp)
set(SRC_WSR88D_RDA source/scwx/wsr88d/rda/clutter_filter_map.cpp
                   source/scwx/wsr88d/rda/digital_radar_data.cpp
                   source/scwx/wsr88d/rda/level2_message.cpp
                   source/scwx/wsr88d/rda/level2_message_factory.cpp
                   source/scwx/wsr88d/rda/level2_message_header.cpp
                   source/scwx/wsr88d/rda/performance_maintenance_data.cpp
                   source/scwx/wsr88d/rda/rda_adaptation_data.cpp
                   source/scwx/wsr88d/rda/rda_status_data.cpp
                   source/scwx/wsr88d/rda/volume_coverage_pattern_data.cpp)
set(HDR_WSR88D_RPG include/scwx/wsr88d/rpg/ccb_header.hpp
                   include/scwx/wsr88d/rpg/cell_trend_data_packet.hpp
                   include/scwx/wsr88d/rpg/cell_trend_volume_scan_times.hpp
                   include/scwx/wsr88d/rpg/digital_precipitation_data_array_packet.hpp
                   include/scwx/wsr88d/rpg/digital_radial_data_array_packet.hpp
                   include/scwx/wsr88d/rpg/generic_data_packet.hpp
                   include/scwx/wsr88d/rpg/graphic_alphanumeric_block.hpp
                   include/scwx/wsr88d/rpg/hda_hail_symbol_packet.hpp
                   include/scwx/wsr88d/rpg/level3_message_header.hpp
                   include/scwx/wsr88d/rpg/linked_contour_vector_packet.hpp
                   include/scwx/wsr88d/rpg/linked_vector_packet.hpp
                   include/scwx/wsr88d/rpg/mesocyclone_symbol_packet.hpp
                   include/scwx/wsr88d/rpg/packet.hpp
                   include/scwx/wsr88d/rpg/packet_factory.hpp
                   include/scwx/wsr88d/rpg/point_feature_symbol_packet.hpp
                   include/scwx/wsr88d/rpg/point_graphic_symbol_packet.hpp
                   include/scwx/wsr88d/rpg/precipitation_rate_data_array_packet.hpp
                   include/scwx/wsr88d/rpg/product_description_block.hpp
                   include/scwx/wsr88d/rpg/product_symbology_block.hpp
                   include/scwx/wsr88d/rpg/radial_data_packet.hpp
                   include/scwx/wsr88d/rpg/raster_data_packet.hpp
                   include/scwx/wsr88d/rpg/scit_forecast_data_packet.hpp
                   include/scwx/wsr88d/rpg/set_color_level_packet.hpp
                   include/scwx/wsr88d/rpg/special_graphic_symbol_packet.hpp
                   include/scwx/wsr88d/rpg/sti_circle_symbol_packet.hpp
                   include/scwx/wsr88d/rpg/storm_id_symbol_packet.hpp
                   include/scwx/wsr88d/rpg/text_and_special_symbol_packet.hpp
                   include/scwx/wsr88d/rpg/unlinked_contour_vector_packet.hpp
                   include/scwx/wsr88d/rpg/unlinked_vector_packet.hpp
                   include/scwx/wsr88d/rpg/vector_arrow_data_packet.hpp
                   include/scwx/wsr88d/rpg/wind_barb_data_packet.hpp
                   include/scwx/wsr88d/rpg/wmo_header.hpp)
set(SRC_WSR88D_RPG source/scwx/wsr88d/rpg/ccb_header.cpp
                   source/scwx/wsr88d/rpg/cell_trend_data_packet.cpp
                   source/scwx/wsr88d/rpg/cell_trend_volume_scan_times.cpp
                   source/scwx/wsr88d/rpg/digital_precipitation_data_array_packet.cpp
                   source/scwx/wsr88d/rpg/digital_radial_data_array_packet.cpp
                   source/scwx/wsr88d/rpg/generic_data_packet.cpp
                   source/scwx/wsr88d/rpg/graphic_alphanumeric_block.cpp
                   source/scwx/wsr88d/rpg/hda_hail_symbol_packet.cpp
                   source/scwx/wsr88d/rpg/level3_message_header.cpp
                   source/scwx/wsr88d/rpg/linked_contour_vector_packet.cpp
                   source/scwx/wsr88d/rpg/linked_vector_packet.cpp
                   source/scwx/wsr88d/rpg/mesocyclone_symbol_packet.cpp
                   source/scwx/wsr88d/rpg/packet.cpp
                   source/scwx/wsr88d/rpg/packet_factory.cpp
                   source/scwx/wsr88d/rpg/point_feature_symbol_packet.cpp
                   source/scwx/wsr88d/rpg/point_graphic_symbol_packet.cpp
                   source/scwx/wsr88d/rpg/precipitation_rate_data_array_packet.cpp
                   source/scwx/wsr88d/rpg/product_description_block.cpp
                   source/scwx/wsr88d/rpg/product_symbology_block.cpp
                   source/scwx/wsr88d/rpg/radial_data_packet.cpp
                   source/scwx/wsr88d/rpg/raster_data_packet.cpp
                   source/scwx/wsr88d/rpg/scit_forecast_data_packet.cpp
                   source/scwx/wsr88d/rpg/set_color_level_packet.cpp
                   source/scwx/wsr88d/rpg/special_graphic_symbol_packet.cpp
                   source/scwx/wsr88d/rpg/sti_circle_symbol_packet.cpp
                   source/scwx/wsr88d/rpg/storm_id_symbol_packet.cpp
                   source/scwx/wsr88d/rpg/text_and_special_symbol_packet.cpp
                   source/scwx/wsr88d/rpg/unlinked_contour_vector_packet.cpp
                   source/scwx/wsr88d/rpg/unlinked_vector_packet.cpp
                   source/scwx/wsr88d/rpg/vector_arrow_data_packet.cpp
                   source/scwx/wsr88d/rpg/wind_barb_data_packet.cpp
                   source/scwx/wsr88d/rpg/wmo_header.cpp)

add_library(wxdata OBJECT ${HDR_COMMON}
                          ${SRC_COMMON}
                          ${HDR_UTIL}
                          ${SRC_UTIL}
                          ${HDR_WSR88D}
                          ${SRC_WSR88D}
                          ${HDR_WSR88D_RDA}
                          ${SRC_WSR88D_RDA}
                          ${HDR_WSR88D_RPG}
                          ${SRC_WSR88D_RPG})

source_group("Header Files\\common"      FILES ${HDR_COMMON})
source_group("Source Files\\common"      FILES ${SRC_COMMON})
source_group("Header Files\\util"        FILES ${HDR_UTIL})
source_group("Source Files\\util"        FILES ${SRC_UTIL})
source_group("Header Files\\wsr88d"      FILES ${HDR_WSR88D})
source_group("Source Files\\wsr88d"      FILES ${SRC_WSR88D})
source_group("Header Files\\wsr88d\\rda" FILES ${HDR_WSR88D_RDA})
source_group("Source Files\\wsr88d\\rda" FILES ${SRC_WSR88D_RDA})
source_group("Header Files\\wsr88d\\rpg" FILES ${HDR_WSR88D_RPG})
source_group("Source Files\\wsr88d\\rpg" FILES ${SRC_WSR88D_RPG})

target_include_directories(wxdata PRIVATE ${Boost_INCLUDE_DIR}
                                          ${HSLUV_C_INCLUDE_DIR}
                                          ${scwx-data_SOURCE_DIR}/include
                                          ${scwx-data_SOURCE_DIR}/source)
target_include_directories(wxdata INTERFACE ${scwx-data_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(wxdata PRIVATE /W3)
endif()

target_link_libraries(wxdata INTERFACE Boost::iostreams
                                       Boost::log
                                       BZip2::BZip2
                                       hsluv-c)

if (WIN32)
    target_link_libraries(wxdata INTERFACE Ws2_32)
endif()

set_target_properties(wxdata PROPERTIES CXX_STANDARD 20
                                        CXX_STANDARD_REQUIRED ON
                                        CXX_EXTENSIONS OFF)
