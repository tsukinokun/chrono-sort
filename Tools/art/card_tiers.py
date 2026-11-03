"""カードの難易度の区分。card_list.py がデッキ JSON の "difficulty" に書き出す。

    やさしい（1）：中学の教科書に載る出来事
    ふつう（2）  ：高校の教科書で太字になる出来事
    むずかしい（3）：それ以外（どちらの一覧にも無いカード）

出題で「やさしい」を選ぶと 1 だけ、「ふつう」は 1 と 2、「むずかしい」は全部が出る。
"""

EASY = """
jushichijo_kenpo kenzuishi taika_no_kaishin hakusonko jinshin_no_ran taiho_ritsuryo heijokyo_sento kojiki nihon_shoki
konden_einen daibutsu_kaigen heiankyo_sento kentoshi_haishi kokin_wakashu michinaga_sessho byodoin insei hogen_no_ran
heiji_no_ran kiyomori_daijo dannoura yoritomo_shogun jokyu_no_ran goseibai_shikimoku bunei_no_eki koan_no_eki
kamakura_metsubo kenmu_shinsei takauji_shogun nanboku_goitsu kinkaku onin_no_ran teppo_denrai xavier_raichi okehazama
muromachi_metsubo nagashino honnoji katanagari tenka_toitsu bunroku_no_eki sekigahara edo_bakufu osaka_natsu shimabara
sakoku kyoho_kaikaku kaitai_shinsho kansei_kaikaku ino_sokuryo oshio_no_ran perry_raiko nichibei_washin nichibei_shuko
sakuradamon taisei_houkan boshin_senso haihan_chiken seinan_senso teikoku_kenpo nisshin_senso nichiro_senso
kanto_daishinsai futsu_senkyo niniroku taiheiyo_senso shusen kenpo_shiko sanfrancisco shinkansen tokyo_olympic_1964
osaka_banpaku okinawa_henkan hanshin_awaji nagano_olympic higashi_nihon kin_in himiko_gi kango_boeki sankin_kotai dejima
tenpo_kaikaku tetsudo_kaigyo naikaku_seido nichiei_domei kokuren_kamei kani_junikai horyuji_soken ganjin buke_shohatto
shimonoseki portsmouth kanzei_jishuken kome_sodo manshu_jihen renmei_dattai nicchu_senso gokajo chiso_kaisei choheirei
gakusei kenpo_kofu hiroshima goichigo sangoku_domei tokyo2020 kankoku_heigo yawata satcho_domei edo_kaijo sangoku_kansho
hara_naikaku tokyo_daikushu chian_iji nisso nicchu_kokko heisei reiwa tokyo_saiban minamata kokkai_kaisetsu jiyuto
minsen_giin iwakura tomioka rokumeikan keisatsu_yobitai jieitai skytree tokyo_tower

first_olympiad marathon death_of_socrates alexander_east qin_unification caesar_assassination vesuvius red_cliffs
edict_of_milan rome_division fall_of_western_rome hijra charlemagne first_crusade genghis_khan magna_carta
hundred_years_war ming_founding fall_of_constantinople columbus vasco_da_gama ninety_five_theses magellan armada
east_india_company mayflower glorious_revolution us_independence bastille napoleon_emperor trafalgar russian_campaign
waterloo opium_war origin_of_species us_civil_war gettysburg_address suez_canal german_empire telephone eiffel_tower
wright_brothers titanic ww1_outbreak russian_revolution penicillin great_depression ww2_outbreak united_nations
prc_founding sputnik cuban_missile apollo11 berlin_wall_fall ussr_dissolution joan_orleans versailles
german_reunification lincoln_assassination athens_1896 xinhai colosseum sui_unification tang_founding louis_xvi
boston_tea watt_engine edison_bulb relativity gagarin bandung india_independence korean_war berlin_wall_built
i_have_a_dream vietnam_end chernobyl hong_kong oil_crisis eu nobel league nato yalta normandy udhr gandhi_death
jfk_assassination sept11 euro covid_who obama tiananmen gulf_war apartheid_end www copernicus communist_manifesto
cleopatra taj_mahal statue_of_liberty tutankhamun rights_of_man washington_president congress_vienna black_death
march_first may_fourth hitler_chancellor new_deal ussr_founding sarajevo jenner salt_march marshall_plan
cultural_revolution xray curie_radium
""".split()

NORMAL = """
kentoshi_first yoritomo_kyohei einin_tokusei kaga_ikko meireki_taika wado_kaichin sanze_isshin kokubunji otenmon
konjikido kongo_rikishi dogen_soto nichiren kenmu_shikimoku ryukyu_okoku nobunaga_joraku hieizan azuchi tensho_shisetsu
bateren_tsuiho keicho_no_eki ryukyu_shinko keicho_shisetsu tanuma laxman rezanov phaeton siebold bansha satsuei kinmon
hanseki_hokan nicchoshuko radio_hoso yukawa fujiwara_kyo nagaokakyo tamuramaro kukai_saicho michizane_sasen shinkokin
hojoki sanetomo oei_no_ran itsukushima anegawa ishiyama shizugatake nijuroku_seijin tsushinshi shakushain okuno_hosomichi
ansei_taigoku kanrin_maru namamugi okinawa_ken teikoku_gikai otsu nijuikkajo renmei_kamei terebi anpo kawabata seikan
iwai nagaya_no_o kusuko shishigatani ichinotani yashima oshu_fujiwara kamakura_daibutsu shocho_ikki ginkaku rikyu
osaka_fuyu toshogu yokohama_kaiko shimonoseki_hogeki hakodate nichigin normanton kyoiku_chokugo suiheisha
sekai_isan kyoto_giteisho wao_bu tendaishu koyasan masakado zenkunen gosannen shugo_jito eisai rokuhara
yoshimitsu_shogun yamashiro_ikki mikatagahara rakuichi_azuchi hideyoshi_kampaku iemitsu_shogun ako_roshi meyasubako
kujikata kansei_igaku mamiya uchiharai nichiro_washin ikedaya osei_fukko kaitakushi yubin gakumon_susume
chishima_karafuto kokato kokkai_kisei kaishinto chichibu tanaka_jikiso hibiya mantetsu seito taisho_seihen siberia
kinyu_kyoko manshukoku sodoinho yokusankai nisso_churitsu midway gakuto_shutsujin kyoiku_kihon dodge_line gojugonen
shotoku_baizo nikkan_kihon kogai_kihon nicchu_heiwa plaza_accord pko_ho sarin seto_ohashi jnr world_cup_2002
noto_jishin kumamoto_jishin kinkyu_jitai osaka_expo2025 aichi_banpaku sapporo_olympic heike_nokyo tdl shohizei
seiken_kotai fuji_whs yamanaka hayabusa chosakurin kinkaikin

thermopylae salamis cannae fall_of_carthage actium nicaea tours treaty_of_verdun otto_coronation hastings granada
aztec_fall thirty_years_war westphalia principia alexander_death zama augustus cai_lun_paper great_schism marco_polo
augsburg lepanto vienna_1683 solon peloponnesian punic_first spartacus rubicon rome_fire yellow_turban hagia_sophia
song_founding canossa saladin baghdad_fall zheng_he dias tordesillas mughal inca_fall trent nantes
charles_execution louis_xiv nerchinsk plassey stockton july_revolution great_exhibition alaska transcontinental
liu_bang later_han sack_of_rome justinian_code umayyad an_lushan ottoman jesuits elizabeth voc later_jin seven_years
cook rosetta louisiana victoria gold_rush crimean spanish_american boxer roc_founding armistice hannibal_alps
shihuang_death hadrians_wall state_religion xuanzang silla wu_zetian abbasid talas goryeo joseon hangul gregorian
don_quixote ming_fall us_constitution egypt_campaign nanjing sepoy serf_emancipation emancipation franco_prussian
model_t amundsen panama cuban_revolution prague_spring iran_revolution lehman galileo_moons dna_helix everest
israel potsdam spanish_flu munich dunkirk stalingrad delian_league grand_canal tang_fall jerusalem_1099 yuan_name
estates_general avignon golden_bull hundred_end sistine charles5 supremacy calvin akbar st_bartholomew
dutch_independence qing_name english_civil bill_of_rights spanish_succession austrian_succession social_contract
catherine2 lexington wealth_nations paris_1783 napoleonic_code haiti_independence austerlitz continental_blockade
monroe feb_revolution taiping arrow_war italy_kingdom red_cross das_kapital paris_commune berlin_congress
berlin_conference triple_entente us_ww1 weimar march_on_rome turkey_republic long_march spanish_civil guernica
molotov_ribbentrop atlantic_charter truman_doctrine berlin_blockade warsaw_pact suez_crisis eec africa_year
civil_rights_act asean six_day nixon_china summit deng_reform afghan_1979 perestroika malta iraq_war brexit
paris_agreement dolly human_genome sumatra_tsunami nuremberg anschluss washington_conf lusitania verdun koch_tb
dreyfus boer_war bloody_sunday kellogg_briand mendeleev lumiere frederick2 peter_embassy galileo_trial descartes
""".split()


def difficulty(card_id):
    """カードの難易度（1〜3）を返す。"""
    if card_id in EASY_SET:
        return 1
    if card_id in NORMAL_SET:
        return 2
    return 3


EASY_SET = set(EASY)
NORMAL_SET = set(NORMAL)
assert not (EASY_SET & NORMAL_SET), sorted(EASY_SET & NORMAL_SET)
assert len(EASY_SET) == len(EASY) and len(NORMAL_SET) == len(NORMAL), "duplicate id in a tier list"
