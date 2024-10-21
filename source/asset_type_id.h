/* date = June 4th 2024 11:42 pm */

#ifndef ASSET_TYPE_ID_H
#define ASSET_TYPE_ID_H

enum Asset_tag_id {
  Tag_smoothness,
  Tag_flatness,
  Tag_facing_dir,
  Tag_unicode_codepoint,
  
  Tag_count,
};

enum Asset_type_id {
  Asset_null,
  
  Asset_shadow,
  Asset_tree,
  Asset_sword,
  //Asset_stairwell,
  Asset_rock,
  
  Asset_grass,
  Asset_tuft,
  Asset_stone,
  
  Asset_head,
  Asset_cape,
  Asset_torso,
  
  Asset_font,
  
  //Asset_monster,
  //Asset_familiar,
  
  ////////////////////////// Sounds
  
  Asset_bloop,
  Asset_crack,
  Asset_drop,
  Asset_glide,
  Asset_music,
  Asset_puhp,
  
  Asset_count
};

#endif //ASSET_TYPE_ID_H
