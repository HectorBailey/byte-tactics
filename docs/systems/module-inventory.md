# System-to-module coverage inventory

[Manual index](index.md)

This inventory contains every distinct folder/module pair in the reference revision's `data/modules.csv`. Chapter assignments are documentation ownership, not claims that all functions in a module have already been analyzed. The manual index distinguishes drafted chapters from planned chapters. Source files sometimes hold functions from more than one conceptual system.

| Folder | Module | Recorded start address(es) | Documentation chapter |
| --- | --- | --- | --- |
| ai | ai_player | 0x406c90 | 15. Computer-player behavior |
| ai | ai_profile | 0x428c60 | 15. Computer-player behavior |
| ai | pathfind | 0x40d5e0 | 08. Ground movement and pathfinding |
| debug | debug_helper | 0x4cbb70 | 20. Diagnostics |
| debug | debug_lib | 0x4d80b0 | 20. Diagnostics |
| debug | memory_stats | 0x47c4d0 | 20. Diagnostics |
| frontend | campaign_menu | 0x476740 | 17. Campaign selection |
| frontend | endgame | 0x41d920 | 17. Endgame |
| frontend | frontend | 0x4256d0 | 14. Frontend states |
| frontend | multi | 0x440d70 | 16. Multiplayer setup |
| frontend | options | 0x45b800 | 14, 19. Options |
| frontend | skirmish_menu | 0x479660 | 14, 15. Skirmish setup |
| game | campaign | 0x435100 | 17. Missions and campaigns |
| game | cd_check | 0x41d4c0 | 01. Archives and content checks |
| game | console_commands | 0x416240 | 20. Console and diagnostics |
| game | data_files | 0x429000 | 01, 13. Asset resolution and visual resources |
| game | economy | 0x401000 | 03. Economy |
| game | game_load | 0x497180 | 01. Match loading |
| game | game_state | 0x490ac0 | 01, 02. Lifecycle and environment |
| game | main | 0x49e640 | 01, 02. Startup and frame dispatch |
| game | main_loop | 0x496790 | 02. Time and simulation |
| game | players | 0x464290 | 05, 16, 18. Players and persistence |
| game | savegame | 0x491ec0 | 18. Save and restore |
| game | settings | 0x42f910 | 01, 14. Configuration |
| game | settings_campaign | 0x432520 | 01, 17. Campaign configuration |
| game | share_commands | 0x418cd0 | 03, 12, 16. Resource and information sharing |
| game | side_data | 0x4318c0 | 01, 05. Side definitions |
| game | statistics | 0x46bce0 | 17. Statistics |
| game | victory | 0x48dfb0 | 17. Victory conditions |
| graphics | blit | 0x4cbbe0 | 13. Rendering and visual effects |
| graphics | display | 0x4b4f50 | 13. Rendering and visual effects |
| graphics | draw | 0x4be950 | 13. Rendering and visual effects |
| graphics | font | 0x4c1480 | 13. Rendering and visual effects |
| graphics | gaf | 0x4b7e60 | 13. Rendering and visual effects |
| graphics | model_render | 0x458160 | 13. Rendering and visual effects |
| graphics | movies | 0x47bdf0, 0x47c6c0 | 13. Rendering and visual effects |
| graphics | palette | 0x4ba200 | 13. Rendering and visual effects |
| graphics | palette_tables | 0x42e140 | 13. Rendering and visual effects |
| graphics | particles | 0x470a40 | 13. Rendering and visual effects |
| graphics | pcx | 0x4caa40 | 13. Rendering and visual effects |
| graphics | surface | 0x4c5d90 | 13. Rendering and visual effects |
| gui | dialogs | 0x4aa8f0 | 14. Interface infrastructure |
| gui | file_list | 0x4aefa0 | 14. Interface infrastructure |
| gui | gui | 0x49f8c0 | 14. Interface infrastructure |
| gui | gui_file | 0x4acd00 | 14. Interface infrastructure |
| gui | keyboard | 0x4c1a40 | 14. Interface infrastructure |
| gui | listbox | 0x4b0090 | 14. Interface infrastructure |
| gui | mouse | 0x4c2380 | 14. Interface infrastructure |
| ingame | build_placement | 0x498f70 | 07, 14. Placement and input |
| ingame | camera | 0x41d0f0 | 14. Camera |
| ingame | control_panel | 0x419560 | 07, 14. Production and controls |
| ingame | dialogs | 0x493340 | 02, 14. Simulation scheduling and dialogs |
| ingame | info_panel | 0x467440 | 12, 13, 14. Sensors and battle display |
| ingame | keys | 0x495860 | 14. Game keys |
| ingame | markers | 0x438c00 | 12, 14. World overlays |
| ingame | screenshots | 0x499890 | 02, 14. Frame dispatch and capture |
| ingame | selection | 0x48bae0 | 14. Selection |
| map | features | 0x421e60 | 04. Features |
| map | line_of_sight | 0x4814c0 | 04, 12. Map loading and sight |
| map | los_tables | 0x432ba0 | 12. Sight tables |
| map | map_list | 0x434a30 | 04, 14. Map metadata and selection |
| map | map_load | 0x435d30 | 04. Map support |
| map | meteors | 0x437cd0 | 04. Environment |
| map | plot_map | 0x484ce0 | 04, 13. Terrain display |
| map | radar | 0x4665d0 | 12, 14. Radar and minimap |
| map | terrain | 0x47c790 | 04, 08. Terrain and occupancy |
| network | hapinet | 0x4c9530 | 16. Multiplayer and synchronization |
| network | net_chat | 0x463ca0 | 16. Multiplayer and synchronization |
| network | net_condenser | 0x44f720 | 16. Multiplayer and synchronization |
| network | net_game | 0x44fd40 | 16. Multiplayer and synchronization |
| network | net_stats | 0x415b20 | 16. Multiplayer and synchronization |
| network | online | 0x45b250 | 16. Multiplayer and synchronization |
| network | packets | 0x460e20 | 16. Multiplayer and synchronization |
| network | unit_sync | 0x46c620 | 16. Multiplayer and synchronization |
| orders | order_dispatch | 0x43f0e0 | 06, 07, 08, 09. Orders and movement |
| orders | order_list | 0x439b30 | 06, 07, 08, 09. Orders and movement |
| orders | order_queue | 0x4384a0 | 06, 07, 08, 09. Orders and movement |
| orders | order_targets | 0x44ce40 | 06, 07, 08, 09. Orders and movement |
| orders | unit_orders | 0x401c20 | 06, 07, 08, 09. Orders and movement |
| orders | vtol_orders | 0x40f200 | 06, 07, 08, 09. Orders and movement |
| runtime | basic_string | 0x4c4fa0 | 20. Runtime support |
| sound | cd_audio | 0x4cd9b0 | 19. Audio |
| sound | play_sound | 0x49f610 | 19. Audio |
| sound | samples | 0x4cee50 | 19. Audio |
| sound | sound | 0x47ed40 | 19. Audio |
| sound | sound_types | 0x42f450 | 19. Audio |
| units | cob | 0x4b0610 | 10. COB execution |
| units | movement_class | 0x440230 | 08. Navigation |
| units | squads | 0x480190 | 05, 15. Squads |
| units | unit_commands | 0x487bf0 | 05, 06. Unit commands |
| units | unit_motion | 0x437840 | 03, 13. Extraction and rendering cache |
| units | unit_position | 0x48a7f0 | 02, 05, 08, 16. Unit updates and synchronization |
| units | unit_save | 0x486fd0 | 18. Unit persistence |
| units | unit_script_calls | 0x4804e0 | 10. Script interfaces |
| units | unit_scripts | 0x43cd20 | 07, 10. Script callbacks and model queries |
| units | unit_targets | 0x489280 | 05, 11. References and combat state |
| units | unit_types | 0x42a8d0 | 01, 05. Definition loading |
| units | units | 0x485010 | 05, 08. Unit and terrain helpers |
| util | commands | 0x4b73b0 | 20. Command infrastructure |
| util | hapibank | 0x4b3630 | 18. Structured persistence |
| util | hpi | 0x4bb190 | 01, 20. Archive and file services |
| util | int_map | 0x4b2290 | 20. Containers |
| util | math | 0x4b6b80 | 08, 11, 20. Math support |
| util | operator_new | 0x4b4f10 | 20. Allocation |
| util | registry | 0x4b6750 | 01, 20. Settings storage |
| util | sqsh | 0x4d0ab0 | 20. Compression |
| util | tdf | 0x4c2ea0, 0x4c5170 | 01. Structured text |
| weapons | explosions | 0x420620 | 11. Combat |
| weapons | weapon_types | 0x42e310 | 11. Combat |
| weapons | weapons | 0x499a30 | 11. Combat |

Total recorded module groups: 110.

Additional source assets and shared infrastructure, including `src/data`, `src/res`, `include`, class headers, and support files such as ballistics, are covered through their consumers. The module table is a coverage baseline, not a substitute for the complete file inventory. Original translation-unit boundaries and names remain hypotheses where the repository says so.
