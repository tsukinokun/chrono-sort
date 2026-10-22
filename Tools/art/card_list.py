"""デッキの元データ（出来事・年・分類・画像の検索語）。

    python Tools/art/card_list.py      デッキ JSON を書き出す

年号は確実なものだけを入れる（諸説ある出来事・年が2年にまたがる出来事は入れない）。
query は Wikimedia Commons での画像の検索語（英語のほうが当たりやすい）。
file を指定すると、検索せずにその Commons のファイルを使う（検索で外れたときの差し替え用）。
"""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DECK_DIR = ROOT / "ChronoSort" / "Assets" / "Data" / "Decks"

# (id, 出来事, 年, 分類, 検索語, 指定ファイル)
JAPAN = [
    ("jushichijo_kenpo", "十七条の憲法", 604, "政治", "Prince Shotoku portrait", None),
    ("kenzuishi", "遣隋使の派遣", 607, "外交", "Horyu-ji", None),
    ("taika_no_kaishin", "大化の改新", 645, "政治", "Nakatomi no Kamatari portrait", None),
    ("hakusonko", "白村江の戦い", 663, "戦争", "Emperor Tenji portrait", None),
    ("jinshin_no_ran", "壬申の乱", 672, "戦争", "Emperor Tenmu", None),
    ("taiho_ritsuryo", "大宝律令の制定", 701, "政治", "Fujiwara-kyo", None),
    ("heijokyo_sento", "平城京遷都", 710, "政治", "Heijo Palace Suzakumon", None),
    ("kojiki", "古事記の成立", 712, "文化", "Kojiki Shinpukuji manuscript", None),
    ("nihon_shoki", "日本書紀の成立", 720, "文化", "Nihon Shoki Tanaka manuscript", None),
    ("konden_einen", "墾田永年私財法", 743, "政治", "Emperor Shomu", None),
    ("daibutsu_kaigen", "東大寺大仏の開眼", 752, "文化", "Todai-ji Daibutsu", None),
    ("heiankyo_sento", "平安京遷都", 794, "政治", "Heian-kyo model", None),
    ("kentoshi_haishi", "遣唐使の廃止", 894, "外交", "Sugawara no Michizane", None),
    ("kokin_wakashu", "古今和歌集の成立", 905, "文化", "Kokin Wakashu manuscript", None),
    ("michinaga_sessho", "藤原道長が摂政に", 1016, "政治", "Fujiwara no Michinaga", None),
    ("byodoin", "平等院鳳凰堂の建立", 1053, "文化", "Byodo-in Phoenix Hall", None),
    ("insei", "院政の始まり", 1086, "政治", "Emperor Shirakawa", None),
    ("hogen_no_ran", "保元の乱", 1156, "戦争", "Hogen rebellion screen", None),
    ("heiji_no_ran", "平治の乱", 1159, "戦争", "Heiji Monogatari Emaki", None),
    ("kiyomori_daijo", "平清盛が太政大臣に", 1167, "政治", "Taira no Kiyomori", None),
    ("dannoura", "壇ノ浦の戦い", 1185, "戦争", "Battle of Dan-no-ura", None),
    ("yoritomo_shogun", "源頼朝が征夷大将軍に", 1192, "政治", "Minamoto no Yoritomo portrait", None),
    ("jokyu_no_ran", "承久の乱", 1221, "戦争", "Emperor Go-Toba", None),
    ("goseibai_shikimoku", "御成敗式目の制定", 1232, "政治", "Hojo Yasutoki", None),
    ("bunei_no_eki", "文永の役", 1274, "戦争", "Moko Shurai Ekotoba", None),
    ("koan_no_eki", "弘安の役", 1281, "戦争", "Mongol invasion of Japan 1281 scroll", None),
    ("kamakura_metsubo", "鎌倉幕府の滅亡", 1333, "戦争", "Siege of Kamakura 1333", None),
    ("kenmu_shinsei", "建武の新政", 1334, "政治", "Emperor Go-Daigo", None),
    ("takauji_shogun", "足利尊氏が征夷大将軍に", 1338, "政治", "Ashikaga Takauji", None),
    ("nanboku_goitsu", "南北朝の合一", 1392, "政治", "Ashikaga Yoshimitsu", None),
    ("kinkaku", "金閣の建立", 1397, "文化", "Kinkaku-ji", None),
    ("onin_no_ran", "応仁の乱", 1467, "戦争", "Onin war", None),
    ("teppo_denrai", "鉄砲伝来", 1543, "技術", "Tanegashima matchlock", None),
    ("xavier_raichi", "ザビエルの来日", 1549, "宗教", "Francis Xavier Kobe City Museum", None),
    ("okehazama", "桶狭間の戦い", 1560, "戦争", "Battle of Okehazama ukiyo-e", None),
    ("muromachi_metsubo", "室町幕府の滅亡", 1573, "政治", "Ashikaga Yoshiaki", None),
    ("nagashino", "長篠の戦い", 1575, "戦争", "Battle of Nagashino screen", None),
    ("honnoji", "本能寺の変", 1582, "戦争", "Honno-ji incident ukiyo-e", None),
    ("katanagari", "刀狩令", 1588, "政治", "Toyotomi Hideyoshi portrait", None),
    ("tenka_toitsu", "豊臣秀吉の天下統一", 1590, "政治", "Siege of Odawara 1590", None),
    ("bunroku_no_eki", "文禄の役", 1592, "戦争", "Siege of Busanjin 1592", None),
    ("sekigahara", "関ヶ原の戦い", 1600, "戦争", "Battle of Sekigahara folding screen", None),
    ("edo_bakufu", "江戸幕府の成立", 1603, "政治", "Tokugawa Ieyasu portrait", None),
    ("osaka_natsu", "大坂夏の陣", 1615, "戦争", "Siege of Osaka screen", None),
    ("shimabara", "島原の乱", 1637, "戦争", "Shimabara Rebellion", None),
    ("sakoku", "ポルトガル船の来航禁止", 1639, "外交", "Nanban ship screen", None),
    ("kyoho_kaikaku", "享保の改革", 1716, "政治", "Tokugawa Yoshimune", None),
    ("kaitai_shinsho", "解体新書の刊行", 1774, "科学", "Kaitai Shinsho", None),
    ("kansei_kaikaku", "寛政の改革", 1787, "政治", "Matsudaira Sadanobu", None),
    ("ino_sokuryo", "伊能忠敬の測量開始", 1800, "科学", "Ino Tadataka map", None),
    ("oshio_no_ran", "大塩平八郎の乱", 1837, "戦争", "Oshio Heihachiro", None),
    ("perry_raiko", "ペリー来航", 1853, "外交", "Black Ships Perry Japanese woodblock", None),
    ("nichibei_washin", "日米和親条約", 1854, "外交", "Perry landing Yokohama 1854 Heine", None),
    ("nichibei_shuko", "日米修好通商条約", 1858, "外交", "Ii Naosuke", None),
    ("sakuradamon", "桜田門外の変", 1860, "政治", "Sakuradamon Incident", None),
    ("taisei_houkan", "大政奉還", 1867, "政治", "Taisei Hokan painting", None),
    ("boshin_senso", "戊辰戦争", 1868, "戦争", "Boshin War", None),
    ("haihan_chiken", "廃藩置県", 1871, "政治", "Emperor Meiji 1872 Uchida", None),
    ("seinan_senso", "西南戦争", 1877, "戦争", "Satsuma Rebellion woodblock", None),
    ("teikoku_kenpo", "大日本帝国憲法の発布", 1889, "政治", "Promulgation of the Meiji Constitution", None),
    ("nisshin_senso", "日清戦争", 1894, "戦争", "First Sino-Japanese War woodblock", None),
    ("nichiro_senso", "日露戦争", 1904, "戦争", "Battle of Tsushima Mikasa Tojo painting", None),
    ("kanto_daishinsai", "関東大震災", 1923, "災害", "1923 Great Kanto earthquake", None),
    ("futsu_senkyo", "普通選挙法の成立", 1925, "政治", "Kato Takaaki", None),
    ("niniroku", "二・二六事件", 1936, "政治", "February 26 Incident", None),
    ("taiheiyo_senso", "太平洋戦争の開戦", 1941, "戦争", "Attack on Pearl Harbor", None),
    ("shusen", "終戦", 1945, "戦争", "Japanese Instrument of Surrender", None),
    ("kenpo_shiko", "日本国憲法の施行", 1947, "政治", "Constitution of Japan 1947 original", None),
    ("sanfrancisco", "サンフランシスコ平和条約", 1951, "外交", "Treaty of San Francisco signing Yoshida", None),
    ("shinkansen", "東海道新幹線の開業", 1964, "技術", "0 series Shinkansen 1964", None),
    ("tokyo_olympic_1964", "東京オリンピック", 1964, "文化", "1964 Summer Olympics opening ceremony", None),
    ("osaka_banpaku", "大阪万博", 1970, "文化", "Tower of the Sun Taro Okamoto", None),
    ("okinawa_henkan", "沖縄返還", 1972, "外交", "Okinawa Reversion Ceremony 1972", None),
    ("hanshin_awaji", "阪神・淡路大震災", 1995, "災害", "Great Hanshin earthquake collapsed expressway", None),
    ("nagano_olympic", "長野オリンピック", 1998, "文化", "1998 Winter Olympics Nagano", None),
    ("higashi_nihon", "東日本大震災", 2011, "災害", "2011 Tohoku earthquake tsunami", None),
]

WORLD = [
    ("first_olympiad", "第1回古代オリンピック", -776, "文化", "Olympia stadium ancient", None),
    ("marathon", "マラトンの戦い", -490, "戦争", "Battle of Marathon painting", None),
    ("thermopylae", "テルモピュライの戦い", -480, "戦争", "Leonidas at Thermopylae David", None),
    ("salamis", "サラミスの海戦", -480, "戦争", "Battle of Salamis Kaulbach", None),
    ("death_of_socrates", "ソクラテスの死", -399, "思想", "The Death of Socrates David", None),
    ("alexander_east", "アレクサンドロスの東方遠征", -334, "戦争", "Alexander Mosaic", None),
    ("qin_unification", "秦の中国統一", -221, "政治", "Qin Shi Huang portrait", None),
    ("cannae", "カンネーの戦い", -216, "戦争", "Hannibal Barca bust", None),
    ("fall_of_carthage", "カルタゴの滅亡", -146, "戦争", "Scipio Aemilianus ruins of Carthage", None),
    ("caesar_assassination", "カエサル暗殺", -44, "政治", "Death of Caesar Camuccini", None),
    ("actium", "アクティウムの海戦", -31, "戦争", "Battle of Actium painting", None),
    ("vesuvius", "ヴェスヴィオ火山の噴火", 79, "災害", "Last Day of Pompeii Bryullov", None),
    ("red_cliffs", "赤壁の戦い", 208, "戦争", "Red Cliff Qiao Zhongchang", None),
    ("edict_of_milan", "ミラノ勅令", 313, "宗教", "Constantine the Great", None),
    ("nicaea", "ニカイア公会議", 325, "宗教", "First Council of Nicaea icon", None),
    ("rome_division", "ローマ帝国の東西分裂", 395, "政治", "Theodosius I", None),
    ("fall_of_western_rome", "西ローマ帝国の滅亡", 476, "政治", "Romulus Augustulus", None),
    ("hijra", "ヒジュラ（聖遷）", 622, "宗教", "Prophet's Mosque Medina 19th century", None),
    ("tours", "トゥール・ポワティエ間の戦い", 732, "戦争", "Battle of Poitiers Steuben", None),
    ("charlemagne", "カール大帝の戴冠", 800, "政治", "Coronation of Charlemagne", None),
    ("treaty_of_verdun", "ヴェルダン条約", 843, "外交", "Treaty of Verdun map", None),
    ("otto_coronation", "オットー1世の戴冠", 962, "政治", "Otto I Holy Roman Emperor", None),
    ("hastings", "ヘイスティングズの戦い", 1066, "戦争", "Bayeux Tapestry", None),
    ("first_crusade", "第1回十字軍", 1096, "戦争", "First Crusade Siege of Jerusalem", None),
    ("genghis_khan", "チンギス・カンの即位", 1206, "政治", "Genghis Khan portrait", None),
    ("magna_carta", "マグナ・カルタ", 1215, "政治", "Magna Carta", None),
    ("hundred_years_war", "百年戦争の開戦", 1337, "戦争", "Battle of Crecy", None),
    ("ming_founding", "明の建国", 1368, "政治", "Hongwu Emperor portrait", None),
    ("fall_of_constantinople", "コンスタンティノープルの陥落", 1453, "戦争", "Fall of Constantinople painting", None),
    ("granada", "グラナダの陥落", 1492, "戦争", "Capitulation of Granada Pradilla", None),
    ("columbus", "コロンブスのアメリカ到達", 1492, "探検", "Landing of Columbus", None),
    ("vasco_da_gama", "ヴァスコ・ダ・ガマのインド到達", 1498, "探検", "Vasco da Gama portrait", None),
    ("ninety_five_theses", "ルターの95か条の論題", 1517, "宗教", "Luther 95 theses", None),
    ("magellan", "マゼランの世界周航への出発", 1519, "探検", "Ferdinand Magellan", None),
    ("aztec_fall", "アステカ帝国の滅亡", 1521, "戦争", "Fall of Tenochtitlan", None),
    ("armada", "アルマダの海戦", 1588, "戦争", "Spanish Armada painting", None),
    ("east_india_company", "イギリス東インド会社の設立", 1600, "経済", "East India House", None),
    ("thirty_years_war", "三十年戦争の開戦", 1618, "戦争", "Defenestration of Prague", None),
    ("mayflower", "メイフラワー号の航海", 1620, "探検", "Mayflower Plymouth Harbor", None),
    ("westphalia", "ウェストファリア条約", 1648, "外交", "Ratification of the Treaty of Munster", None),
    ("principia", "ニュートン『プリンキピア』", 1687, "科学", "Newton Principia title page", None),
    ("glorious_revolution", "名誉革命", 1688, "政治", "William III landing at Brixham", None),
    ("us_independence", "アメリカ独立宣言", 1776, "政治", "Declaration of Independence Trumbull", None),
    ("bastille", "バスティーユ襲撃", 1789, "政治", "Storming of the Bastille", None),
    ("napoleon_emperor", "ナポレオンの皇帝即位", 1804, "政治", "Coronation of Napoleon David", None),
    ("trafalgar", "トラファルガーの海戦", 1805, "戦争", "Battle of Trafalgar Turner", None),
    ("russian_campaign", "ナポレオンのロシア遠征", 1812, "戦争", "Napoleon retreat from Moscow", None),
    ("waterloo", "ワーテルローの戦い", 1815, "戦争", "Battle of Waterloo painting", None),
    ("opium_war", "アヘン戦争の開戦", 1840, "戦争", "First Opium War Nemesis", None),
    ("origin_of_species", "『種の起源』の出版", 1859, "科学", "On the Origin of Species title page", None),
    ("us_civil_war", "南北戦争の開戦", 1861, "戦争", "Bombardment of Fort Sumter", None),
    ("gettysburg_address", "ゲティスバーグ演説", 1863, "政治", "Abraham Lincoln 1863", None),
    ("suez_canal", "スエズ運河の開通", 1869, "技術", "Suez Canal inauguration 1869", None),
    ("german_empire", "ドイツ帝国の成立", 1871, "政治", "Proclamation of the German Empire Werner", None),
    ("telephone", "ベルの電話の特許", 1876, "技術", "Alexander Graham Bell telephone", None),
    ("eiffel_tower", "エッフェル塔の完成", 1889, "技術", "Eiffel Tower 1889", None),
    ("wright_brothers", "ライト兄弟の初飛行", 1903, "技術", "Wright Flyer first flight 1903", None),
    ("titanic", "タイタニック号の沈没", 1912, "災害", "RMS Titanic", None),
    ("ww1_outbreak", "第一次世界大戦の開戦", 1914, "戦争", "World War I trenches", None),
    ("russian_revolution", "ロシア革命", 1917, "政治", "October Revolution 1917", None),
    ("penicillin", "ペニシリンの発見", 1928, "科学", "Alexander Fleming", None),
    ("great_depression", "世界恐慌の始まり", 1929, "経済", "Wall Street crowd 1929 crash", None),
    ("ww2_outbreak", "第二次世界大戦の開戦", 1939, "戦争", "Schleswig-Holstein Westerplatte 1939", None),
    ("united_nations", "国際連合の発足", 1945, "外交", "United Nations Charter signing 1945", None),
    ("prc_founding", "中華人民共和国の成立", 1949, "政治", "Founding ceremony of the People's Republic of China 1949", None),
    ("sputnik", "スプートニク1号の打ち上げ", 1957, "科学", "Sputnik 1", None),
    ("cuban_missile", "キューバ危機", 1962, "外交", "Cuban Missile Crisis", None),
    ("apollo11", "アポロ11号の月面着陸", 1969, "科学", "Aldrin Apollo 11", None),
    ("berlin_wall_fall", "ベルリンの壁の崩壊", 1989, "政治", "Berlin Wall 1989", None),
    ("ussr_dissolution", "ソビエト連邦の崩壊", 1991, "政治", "Belavezha Accords signing", None),
    ("joan_orleans", "ジャンヌ・ダルクのオルレアン解放", 1429, "戦争", "Joan of Arc at the Siege of Orleans Lenepveu", None),
    ("copernicus", "コペルニクス『天球回転論』", 1543, "科学", "Copernicus heliocentric diagram De revolutionibus", None),
    ("galileo_moons", "ガリレオが木星の衛星を発見", 1610, "科学", "Galileo Sidereus Nuncius", None),
    ("versailles", "ヴェルサイユ条約", 1919, "外交", "Signing of Peace in the Hall of Mirrors Orpen", None),
    ("german_reunification", "東西ドイツの統一", 1990, "政治", "Brandenburg Gate 3 October 1990", None),
]


def year_label(year):
    return f"紀元前{-year}年" if year < 0 else f"{year}年"


def write_deck(name, rows, region):
    cards = []
    seen = set()
    for card_id, title, year, category, query, file in rows:
        assert card_id not in seen, card_id
        seen.add(card_id)
        card = {
            "id": card_id,
            "title": title,
            "year": year,
            "yearLabel": year_label(year),
            "region": region,
            "category": category,
            "art": "",
            "artQuery": query,
        }
        if file:
            card["artFile"] = file
        cards.append(card)
    with open(DECK_DIR / f"{name}.json", "w", encoding="utf-8", newline="\r\n") as f:
        json.dump({"deck": name, "cards": cards}, f, ensure_ascii=False, indent=4)
        f.write("\n")
    return len(cards)


def main():
    n = write_deck("japan", JAPAN, "JP")
    m = write_deck("world", WORLD, "WORLD")
    print(f"japan {n} cards, world {m} cards, total {n + m}")


if __name__ == "__main__":
    main()
