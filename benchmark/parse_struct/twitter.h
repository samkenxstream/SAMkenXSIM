namespace {
  using namespace std;
}

namespace twitter {

// https://developer.twitter.com/en/docs/tweets/search/api-reference/get-search-tweets

// ISO 639-1 language code. 2 characters, always.
struct iso_language_code {
  char language_code[2];
};

// This will only ever have ASCII
struct utc_datetime {
  std::string str;
};

struct tweet_source {
  type type;
  std::string source_text;
};

struct expanded_url {
                        // "url": "http://t.co/JTFjV89eaN",
  string expanded_url;  // "expanded_url": "http://www.pixiv.net/member.php?id=1778417",
  string display_url;   // "display_url": "pixiv.net/member.php?id=…",
};
struct expanded_url_substring : expanded_url {
                        // "indices": [
  uint16_t start;       //   0,
  uint16_t end;         //   22
                        // ]
};

struct substring {              //       {
                                        //         "text": "LEDカツカツ選手権",
                                        //         "indices": [
  uint16_t start;                       //           17,
  uint16_t end;                         //           28
                                        //         ]
};                                      //       }

struct user_mention_substring {                                            //       {
                                            //         "screen_name": "thsc782_407",
  string name;                              //         "name": "[90]青葉台  芦 (第二粟屋) 屋",
  string id;                                //         "id": 82900665,
  string id_str;                            //         "id_str": "82900665",
                                            //         "indices": [
  uint16_t start;                           //           3,
  uint16_t end;                             //           15
                                            //         ]
};                                          //       }

// https://developer.twitter.com/en/docs/tweets/data-dictionary/overview/user-object
struct user {                           // "user": {
  uint64_t id;                          //   "id": 114786346,
                                        //   "id_str": "114786346",
  string name;                          //   "name": "RT&ファボ魔のむっつんさっm",
  string screen_name;                   //   "screen_name": "yuttari1998",
  string location;                      //   "location": "静岡県長泉町",
  string description;                   //   "description": "RTしてTLに濁流を起こすからフォローしない方が良いよ 言ってることもつまらないし 詳細→http://t.co/ANSFlYXERJ 相方@1life_5106_hshd 葛西教徒その壱",
  entity_string url;                    //   "url": "http://t.co/JTFjV89eaN",
                                        //   "entities": {
                                        //     "url": {
                                        //       "urls": [
                                        //         {
  optional<expanded_url> expanded_url;  //           "url": "http://t.co/JTFjV89eaN",
                                        //           "expanded_url": "http://www.pixiv.net/member.php?id=1778417",
                                        //           "display_url": "pixiv.net/member.php?id=…",
                                        //           "indices": [
                                        //             0,
                                        //             22
                                        //           ]
                                        //         }
                                        //       ]
                                        //     },
  vector<expanded_url_substring> expanded_description_urls;
                                        //     "description": {
                                        //       "urls": [
                                        //         {
                                        //           "url": "http://t.co/ANSFlYXERJ",
                                        //           "expanded_url": "http://twpf.jp/chibu4267",
                                        //           "display_url": "twpf.jp/chibu4267",
                                        //           "indices": [
                                        //             45,
                                        //             67
                                        //           ]
                                        //         }
                                        //       ]
                                        //     }
                                        //   },
  bool protected;                       //   "protected": false,
  uint32_t followers_count;             //   "followers_count": 1324,
  uint32_t friends_count;               //   "friends_count": 1165,
  uint32_t listed_count;                //   "listed_count": 99,
  utc_datetime created_at;              //   "created_at": "Mon Oct 17 08:23:46 +0000 2011",
  uint32_t favourites_count;            //   "favourites_count": 9542,
  int32_t utc_offset;                   //   "utc_offset": 32400,
  // TODO totally enum-able
  string time_zone;                     //   "time_zone": "Tokyo",
  bool geo_enabled;                     //   "geo_enabled": true,
  bool verified;                        //   "verified": false,
  uint32_t statuses_count;              //   "statuses_count": 369420,
  iso_language_code lang;               //   "lang": "ja",
  bool contributors_enabled;            //   "contributors_enabled": false,
  bool is_translator;                   //   "is_translator": false,
  bool is_translation_enabled;          //   "is_translation_enabled": false,
  uint32_t profile_background_color;    //   "profile_background_color": "C0DEED",
  string profile_background_image_url;  //   "profile_background_image_url": "http://pbs.twimg.com/profile_background_images/453106940822814720/PcJIZv43.png",_
                                        //   "profile_background_image_url_https": "https://pbs.twimg.com/profile_background_images/453106940822814720/PcJIZv43.png",
  bool profile_background_tile;         //   "profile_background_tile": true,
  string profile_image_url;             //   "profile_image_url": "http://pbs.twimg.com/profile_images/505731759216943107/pzhnkMEg_normal.jpeg",
                                        //   "profile_image_url_https": "https://pbs.twimg.com/profile_images/505731759216943107/pzhnkMEg_normal.jpeg",
  string profile_banner_url;            //   "profile_banner_url": "https://pbs.twimg.com/profile_banners/392585658/1362383911",
  uint32_t profile_link_color;          //   "profile_link_color": "5EB9FF",
  uint32_t profile_sidebar_border_color;//   "profile_sidebar_border_color": "FFFFFF",
  uint32_t profile_sidebar_fill_color;  //   "profile_sidebar_fill_color": "DDEEF6",
  uint32_t profile_text_color;          //   "profile_text_color": "333333",
  bool profile_use_background_image;    //   "profile_use_background_image": true,
  bool default_profile;                 //   "default_profile": false,
  bool default_profile_image;           //   "default_profile_image": false,
  bool following;                       //   "following": false,
  bool follow_request_sent;             //   "follow_request_sent": false,
  bool notifications;                   //   "notifications": false
};                                      // }

enum class result_type {
  recent,
  mixed,
  popular
};

struct metadata {                         // "metadata": {
  result_type result_type;                //   "result_type": "recent",
  iso_language_code iso_language_code;    //   "iso_language_code": "ja"
};                                        // }

enum class place_type {
  city
};

struct country_code {
  char code[2];
};

enum class media_type {
  photo
};

enum class media_resize {
  fit,
  crop
};

struct media_size {                         //
  string name;                              //           "medium": {
  uint32_t width;                           //             "w": 600,
  uint32_t height;                          //             "h": 450,
  media_resize resize;                      //             "resize": "fit"
};                                          //           },

struct media_substr {                       //       {
  uint64_t id;                              //         "id": 439430848194936800,
                                            //         "id_str": "439430848194936832",
                                            //         "indices": [
  uint16_t start;                           //           58,
  uint16_t end;                             //           80
                                            //         ],
  string media_url;                         //         "media_url": "http://pbs.twimg.com/media/BhksBzoCAAAJeDS.jpg",
                                            //         "media_url_https": "https://pbs.twimg.com/media/BhksBzoCAAAJeDS.jpg",
  string url;                               //         "url": "http://t.co/vmrreDMziI",
                                            //         "display_url": "pic.twitter.com/vmrreDMziI",
  optional<expanded_url> expanded_url;      //         "expanded_url": "http://twitter.com/thsc782_407/status/439430848190742528/photo/1",
  media_type type;                          //         "type": "photo",
  vector<size> sizes;                       //         "sizes": {
                                            //           "medium": {
                                            //             "w": 600,
                                            //             "h": 450,
                                            //             "resize": "fit"
                                            //           },
                                            //           "large": {
                                            //             "w": 1024,
                                            //             "h": 768,
                                            //             "resize": "fit"
                                            //           },
                                            //           "thumb": {
                                            //             "w": 150,
                                            //             "h": 150,
                                            //             "resize": "crop"
                                            //           },
                                            //           "small": {
                                            //             "w": 340,
                                            //             "h": 255,
                                            //             "resize": "fit"
                                            //           }
                                            //         },
  uint64_t source_status_id;                //         "source_status_id": 439430848190742500,
                                            //         "source_status_id_str": "439430848190742528"
};                                          //       }

struct bounding_box {                         //     "bounding_box": {
                                              //       "type": "Polygon",
  vector<lat_long> coordinates;               //       "coordinates": [
                                              //         [
                                              //           [
                                              //             -74.026675,
                                              //             40.683935
                                              //           ],
                                              //           [
                                              //             -74.026675,
                                              //             40.877483
                                              //           ],
                                              //           [
                                              //             -73.910408,
                                              //             40.877483
                                              //           ],
                                              //           [
                                              //             -73.910408,
                                              //             40.683935
                                              //           ]
                                              //         ]
                                              //       ]
};                                            //     },
                                            //       40.74118764,
                                            //       -73.9998279

struct lat_long {                           //     [
  float lat;                                //       40.74118764,
  float long;                               //       -73.9998279
};                                          //     ]

struct place {                                //   "place": {
  string id;                                  //     "id": "01a9a39529b27f36",
  string url;                                 //     "url": "https://api.twitter.com/1.1/geo/id/01a9a39529b27f36.json",
  place_type place_type;                      //     "place_type": "city",
  string name;                                //     "name": "Manhattan",
  string full_name;                           //     "full_name": "Manhattan, NY",
  country_code country_code;                  //     "country_code": "US",
  string country;                             //     "country": "United States",
  bounding_box bounding_box;                  //     "bounding_box": { ... }
};                                            //   }

struct tweet {                              // {
  tweet_metadata metadata;                  //   "metadata": { ... },
  iso_datetime created_at;                  //   "created_at": "Sun Aug 31 00:29:15 +0000 2014",
  uint64_t id;                              //   "id": 505874924095815700,
                                            //   "id_str": "505874924095815681",
  string text;                              //   "text": "@aym0566x \n\n名前:前田あゆみ\n第一印象:なんか怖っ！\n今の印象:とりあえずキモい。噛み合わない\n好きなところ:ぶすでキモいとこ😋✨✨\n思い出:んーーー、ありすぎ😊❤️\nLINE交換できる？:あぁ……ごめん✋\nトプ画をみて:照れますがな😘✨\n一言:お前は一生もんのダチ💖",
  // Note: there only appear to be three main sources, even though the text is complex:
  // - twitter.com: <a href=\"https://twitter.com/yabai_giness\" rel=\"nofollow\">ヤバすぎる!!ギネス世界記録</a>
  // - android: <a href=\"http://twitter.com/download/android\" rel=\"nofollow\">Twitter for Android</a>
  // - iphone: <a href=\"http://twitter.com/download/iphone\" rel=\"nofollow\">Twitter for iPhone</a>
  // This would benefit massively from uniqification, even more with a bit of smarts to detect twitter.com/USERNAME
  string source;                            //   "source": "<a href=\"http://twitter.com/download/iphone\" rel=\"nofollow\">Twitter for iPhone</a>",
  bool truncated;                           //   "truncated": false,
  optional<user> in_reply_to;               //   "in_reply_to_user_id": 866260188,
                                            //   "in_reply_to_user_id_str": "866260188",
                                            //   "in_reply_to_screen_name": "aym0566x",
  optional<uint64_t> in_reply_to_status_id; //   "in_reply_to_status_id": 505874728897085440,
                                            //   "in_reply_to_status_id_str": "505874728897085440",
  user user;                                //   "user": { ... },
  uint32_t retweet_count;                   //   "retweet_count": 3291,
  uint32_t favorite_count;                  //   "favorite_count": 0,
                                            //   "entities": {
  vector<substring> hashtags;               //     "hashtags": [ ... ],
                                            //     "symbols": [],
  vector<expanded_url_substring> urls;      //     "urls": [],
  vector<user_mention_substring> user_mentions;
                                            //     "user_mentions": [ ... ],
  vector<media> media;                      //     "media": [ ... ],
  optional<lat_long> coordinates;           //   "geo": {
                                            //     "type": "Point",
                                            //     "coordinates": [
                                            //       40.74118764,
                                            //       -73.9998279
                                            //     ]
                                            //   },
                                            //   "coordinates": {
                                            //     "type": "Point",
                                            //     "coordinates": [
                                            //       -73.9998279,
                                            //       40.74118764
                                            //     ]
                                            //   },
  optional<place> place;                    //   "place": { ... },
                                            //   "contributors": null,
  optional<tweet> retweeted_status;         //   "retweeted_status": { ... }
  bool favorited;                           //   "favorited": false,
  bool retweeted;                           //   "retweeted": false,
  bool possibly_sensitive;                  //   "possibly_sensitive": false,
  iso_language_code lang;                   //   "lang": "ja"
};                                          // },


struct search_metadata {                    //   "search_metadata": {
  float completed_in;                       //     "completed_in": 0.087,
  uint64_t max_id;                          //     "max_id": 505874924095815700,
                                            //     "max_id_str": "505874924095815681",
  string next_results;                      //     "next_results": "?max_id=505874847260352512&q=%E4%B8%80&count=100&include_entities=1",
  string query;                             //     "query": "%E4%B8%80",
  string refresh_url;                       //     "refresh_url": "?since_id=505874924095815681&q=%E4%B8%80&include_entities=1",
  uint32_t count;                           //     "count": 100,
  uint64_t since_id;                        //     "since_id": 0,
                                            //     "since_id_str": "0"
};                                          //   }

struct search_tweets {                      // {
  vector<status> statuses;                  //   "statuses": [ ... ],
  search_metadata search_metadata;          //   "search_metadata": { ... }
};                                          // }

} // namespace twitter