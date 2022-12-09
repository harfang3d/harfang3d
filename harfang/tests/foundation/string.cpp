// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/string.h"
#include "foundation/utf8-cpp/utf8.h"
#include "foundation/utf8.h"

using namespace hg;

void test_string() {
	TEST_CHECK(to_lower('A') == 'a');
	TEST_CHECK(to_lower('G') == 'g');
	TEST_CHECK(to_lower('z') == 'z');
	TEST_CHECK(to_lower('e') == 'e');
	TEST_CHECK(to_lower('0') == '0');
	TEST_CHECK(to_lower('!') == '!');

	TEST_CHECK(case_sensitive_eq_func('I', 'I'));
	TEST_CHECK(case_sensitive_eq_func('f', 'f'));
	TEST_CHECK(case_sensitive_eq_func('F', 'f') == false);
	TEST_CHECK(case_sensitive_eq_func('y', 'A') == false);
	TEST_CHECK(case_sensitive_eq_func('A', 'a') == false);
	TEST_CHECK(case_sensitive_eq_func('_', '_'));
	TEST_CHECK(case_sensitive_eq_func('!', '=') == false);

	TEST_CHECK(case_insensitive_eq_func('J', 'J'));
	TEST_CHECK(case_insensitive_eq_func('o', 'o'));
	TEST_CHECK(case_insensitive_eq_func('H', 'h'));
	TEST_CHECK(case_insensitive_eq_func('b', 'B'));
	TEST_CHECK(case_insensitive_eq_func('w', 'L') == false);
	TEST_CHECK(case_insensitive_eq_func('U', 'e') == false);
	TEST_CHECK(case_insensitive_eq_func('*', '*'));
	TEST_CHECK(case_insensitive_eq_func('$', '~') == false);

	TEST_CHECK(starts_with("starts_with", "start"));
	TEST_CHECK(starts_with("StArTs_WiTh", "start") == false);
	TEST_CHECK(starts_with("cAsE_SeNsItIviTy::insensitive", "CaSe_sEnSiTiViTy:", case_sensitivity::insensitive));
	TEST_CHECK(starts_with("Lorem ipsum dolor sit amet", "ipsum", case_sensitivity::insensitive) == false);

	TEST_CHECK(ends_with("starts_with", "_with"));
	TEST_CHECK(ends_with("StArTs_WiTh", "_WIth") == false);
	TEST_CHECK(ends_with("cAsE_SeNsItIviTy::iNsEnSiTiVe", ":InSEnsITiVE", case_sensitivity::insensitive));
	TEST_CHECK(ends_with("Lorem ipsum dolor sit amet", " sit amet", case_sensitivity::insensitive));
	TEST_CHECK(ends_with("Lorem ipsum dolor sit amet", " amet sit", case_sensitivity::insensitive) == false);

	std::string str = "bobcat dogfights a catfish over a hotdog";
	TEST_CHECK(replace_all(str, "dog", "cat") == 2);
	TEST_CHECK(str == "bobcat catfights a catfish over a hotcat");
	TEST_CHECK(replace_all(str, "cat", "DOG") == 4);
	TEST_CHECK(str == "bobDOG DOGfights a DOGfish over a hotDOG");

	std::vector<std::string> list(4);
	list[0] = "bobcat";
	list[1] = "catfish";
	list[2] = "hotdog";
	list[3] = "dogfish";
	TEST_CHECK(split("bobcat  , catfish,hotdog , dogfish ", ",", " ") == list);
	TEST_CHECK(split("*bobcat*||*catfish*||*hotdog*||*dogfish*", "||", "*") == list);
	
	TEST_CHECK(lstrip("Thopha saccata") == "Thopha saccata");
	TEST_CHECK(lstrip("     Baorisa hieroglyphica") == "Baorisa hieroglyphica");
	TEST_CHECK(lstrip("\t\t    \tStigmodera cancellata", " \t") == "Stigmodera cancellata");
	TEST_CHECK(lstrip(" - Stigmodera cancellata", " ") != "Stigmodera cancellata");
	TEST_CHECK(lstrip("  ____    __ _", " _") == std::string());

	TEST_CHECK(rstrip("Selenocosmia crassipes") == "Selenocosmia crassipes");
	TEST_CHECK(rstrip("Agrias claudina    ") == "Agrias claudina");
	TEST_CHECK(rstrip("Mormolyce phyllodes...;;-;..-_-", "_.-;") == "Mormolyce phyllodes");
	TEST_CHECK(rstrip("Phymateus viridipes\n\n ", " \t") != "Stigmodera cancellata");
	TEST_CHECK(rstrip("@ ++ @ @@ ", " @+") == std::string());

	TEST_CHECK(strip("Ornithoptera euphorion") == "Ornithoptera euphorion");
	TEST_CHECK(strip("    Phyllium bioculatum        ") == "Phyllium bioculatum");
	TEST_CHECK(strip("\"0o. .o0\" Eupholus schoenherrii \"0o. .o0\"", "0.\"o ") == "Eupholus schoenherrii");
	TEST_CHECK(strip("<:_Chrysis ruddii= />", "<:/>") == "_Chrysis ruddii= ");

	TEST_CHECK(lstrip_space("Myrmecia brevinoda") == "Myrmecia brevinoda");
	TEST_CHECK(lstrip_space("\n\t\t  Rhachoepalpus metallicus\r\n") == "Rhachoepalpus metallicus\r\n");
	TEST_CHECK(lstrip_space("\r\n\t* Julodis cirrosa") != "Julodis cirrosa");

	TEST_CHECK(rstrip_space("Endoxyla cinerea") == "Endoxyla cinerea");
	TEST_CHECK(rstrip_space("Alaruasa violacea    \t \t  \r\n") == "Alaruasa violacea");
	TEST_CHECK(rstrip_space("Dynastes hercule    \t .\t  \r\n") != "Dynastes hercule");

	TEST_CHECK(strip_space("Phellus olgae") == "Phellus olgae");
	TEST_CHECK(strip_space("\t\t\tProtaetia affinis   \t\t   \r\n ") == "Protaetia affinis");
	TEST_CHECK(strip_space(" * Phobaeticus serratipes\r\n_ ") != "Phobaeticus serratipes");

	TEST_CHECK(reduce("   The Irish   damselfly or  crescent bluet (Coenagrion      lunulatum) is a damselfly found in  northern  Europe and Asia  to "
					  "north-eastern  China      ") ==
			   "The Irish damselfly or crescent bluet (Coenagrion lunulatum) is a damselfly found in northern Europe and Asia to north-eastern China");
	TEST_CHECK(reduce("The Arctic bluet (Coenagrion johanssoni) is found in Northern Europe, and east through Asia as far as the Amur River") ==
			   "The Arctic bluet (Coenagrion johanssoni) is found in Northern Europe, and east through Asia as far as the Amur River");
	TEST_CHECK(reduce("\t  Pyrrhosoma nymphula can reach\ta body length\r\n   of 33-36 millimetres (1.3-1.4 in).\r\n\r\n\r\n", "-=-", " \r\n\t") ==
			   "Pyrrhosoma-=-nymphula-=-can-=-reach-=-a-=-body-=-length-=-of-=-33-36-=-millimetres-=-(1.3-1.4-=-in).");

	const char *club_tailed_dragonflies[] = {
		"Yellow-legged",
		"Club-tailed",
		"Green club-tailed",
		"Green-eyed hook-tailed",
	};
	std::string v = join(club_tailed_dragonflies, club_tailed_dragonflies + 4, " dragonfly, ", "dragonfly.");
	TEST_CHECK(join(club_tailed_dragonflies, club_tailed_dragonflies + 4, " Dragonfly\n") ==
			   "Yellow-legged Dragonfly\nClub-tailed Dragonfly\nGreen club-tailed Dragonfly\nGreen-eyed hook-tailed");
	TEST_CHECK(join(club_tailed_dragonflies, club_tailed_dragonflies + 1, " // ") == "Yellow-legged");
	TEST_CHECK(join(club_tailed_dragonflies, club_tailed_dragonflies, "-", "Plane") == std::string());

	const char *path[] = {
		"e:",
		"hg-core",
		"foundation",
		"string",
		"h",
	};
	TEST_CHECK(join(path, path + 5, "/", ".") == "e:/hg-core/foundation/string.h");
	TEST_CHECK(join(path, path + 1, "\\", ".") == "e:");
	
	TEST_CHECK(contains("CTA TTA TTA ACA AGA AGT ATA GTA GAA AAC GGA GCT GGA ACA GGT TGA ACT GTT TAT CCT CCT CTT TCA TCT AAT ATT", "GGA"));
	TEST_CHECK(contains("GCC CAT AGA GGA GCT TCT GTT GAT TTA GCT ATT TTT TCT CTT CAT TTA GCT GGA ATT TCT TCC ATC CTA GGA GCA GTA ", "ATA") == false);
	TEST_CHECK(contains("", "ATT") == false);
	TEST_CHECK(contains("AGT TTA GTT ACT CAA CGT", "") == true);

	const wchar_t blueberry_jam_utf16_raw[] = {0x0042, 0x006c, 0x00e5, 0x0062, 0x00e6, 0x0072, 0x0073, 0x0079, 0x006c, 0x0074, 0x0065, 0x0074, 0x00f8, 0x0079, 0x0000};
	const char32_t blueberry_jam_utf32_raw[] = {0x00000042, 0x0000006c, 0x000000e5, 0x00000062, 0x000000e6, 0x00000072u, 0x00000073, 0x00000079, 0x0000006c,
		0x00000074, 0x00000065, 0x00000074, 0x000000f8, 0x00000079, 0x00000000};
	
	const std::string blueberry_jam_ansi = "\x42\x6C\xE5\x62\xE6\x72\x73\x79\x6C\x74\x65\x74\xF8\x79\x00";
	const std::string blueberry_jam_utf8 = "\x42\x6C\xC3\xA5\x62\xC3\xA6\x72\x73\x79\x6C\x74\x65\x74\xC3\xB8\x79";
	const std::wstring blueberry_jam_utf16(blueberry_jam_utf16_raw);
	const std::u32string blueberry_jam_utf32(blueberry_jam_utf32_raw);
	
	TEST_CHECK(utf32_to_utf8(blueberry_jam_utf32) == blueberry_jam_utf8);
	TEST_CHECK(utf8_to_utf32(blueberry_jam_utf8) == blueberry_jam_utf32);
	TEST_CHECK(wchar_to_utf8(blueberry_jam_utf16) == blueberry_jam_utf8);
	TEST_CHECK(utf8_to_wchar(blueberry_jam_utf8) == blueberry_jam_utf16);

#if !__linux                                                                    // [todo] fix ansi conversion on *nix
	TEST_CHECK(ansi_to_utf8(blueberry_jam_ansi) == blueberry_jam_utf8);
	TEST_CHECK(ansi_to_wchar(blueberry_jam_ansi) == blueberry_jam_utf16);
#endif

	const char32_t blueberry_jam_utf32_invalid_cp_raw[] = {0x00000042, 0x0000006c, 0x000000e5, 0x00000062, 0x000000e6, 0x00000072u, 0x00000073, 0x00000079,
		0x0000006c, 0x00000074, 0x00000065, 0x00000074, 0x000000f8, 0x00000079, 0x0000ffff};
	const std::string blueberry_jam_utf8_invalid = "\x42\x6C\xC3\xA5\x62\xC3\xa0\xa1\xA6\x72\x73\x79\xf0\x28\x8c\x28\x6C\x74\x65\x74\xC3\xB8\x79";
	const std::u32string blueberry_jam_utf32_invalid_cp(blueberry_jam_utf32_invalid_cp_raw);
		
#ifdef __MSYS__
	TEST_EXCEPTION(utf32_to_utf8(blueberry_jam_utf32_invalid_cp), utf8::invalid_code_point);
#endif
	TEST_EXCEPTION(utf8_to_utf32(blueberry_jam_utf8_invalid), utf8::invalid_utf8);
	TEST_EXCEPTION(utf8_to_wchar(blueberry_jam_utf8_invalid), utf8::invalid_utf8);

	const std::string s_low = "lorem ipsum dolor sit amet, consectetur adipiscing elit";
	std::string s;
	s = "Lorem Ipsum doloR sit AMET, consECtETur adipISCINg eliT";
	TEST_CHECK(tolower(s) == s_low);
	tolower_inplace(s);
	TEST_CHECK(s == s_low);

	s = s_low;
	TEST_CHECK(tolower(s) == s_low);
	tolower_inplace(s);
	TEST_CHECK(s == s_low);

	const std::string s_lo_20_36 = "Lorem Ipsum doloR sit amet, consecteTur adipISCINg eliT";
	s = "Lorem Ipsum doloR sit AMET, CONsecTETur adipISCINg eliT";
	TEST_CHECK(tolower(s, 20, 36) == s_lo_20_36);
	tolower_inplace(s, 20, 36);
	TEST_CHECK(s == s_lo_20_36);

	const std::string s_up = "LOREM IPSUM DOLOR SIT AMET, CONSECTETUR ADIPISCING ELIT";
	s = "Lorem Ipsum doloR sit AMET, consECtETur adipISCINg eliT";
	TEST_CHECK(toupper(s) == s_up);
	toupper_inplace(s);
	TEST_CHECK(s == s_up);

	s = s_up;
	TEST_CHECK(toupper(s) == s_up);
	toupper_inplace(s);
	TEST_CHECK(s == s_up);

	const std::string s_up_20_36 = "Lorem Ipsum doloR siT AMET, CONSECTETur adipISCINg eliT";
	s = "Lorem Ipsum doloR sit Amet, CONsecTETur adipISCINg eliT";
	TEST_CHECK(toupper(s, 20, 36) == s_up_20_36);
	toupper_inplace(s, 20, 36);
	TEST_CHECK(s == s_up_20_36);

	const std::string in = "At vero eos et accusamus et iusto odio dignissimos";
	TEST_CHECK(slice(in, 15) == "accusamus et iusto odio dignissimos");
	TEST_CHECK(slice(in, 3, 8) == "vero eos");
	TEST_CHECK(slice(in, 8, 120) == "eos et accusamus et iusto odio dignissimos");
	TEST_CHECK(slice(in, -16) == "odio dignissimos");
	TEST_CHECK(slice(in, -35, 9) == "accusamus");
	TEST_CHECK(slice(in, -200) == in);
	TEST_CHECK(slice(in, -200, 7) == "At vero");
	TEST_CHECK(slice(in, 25, -17) == "et iusto");
	TEST_CHECK(slice(in, -16, -6) == "odio digni");
	TEST_CHECK(slice(in, 0, -128) == "");
	TEST_CHECK(slice(in, 50, 0) == "");

	TEST_CHECK(left(in, 12) == "At vero eos ");
	TEST_CHECK(left(in, -23) == "At vero eos et accusamus et");
	TEST_CHECK(left(in, -99) == "");
	TEST_CHECK(left(in, 111) == in);

	TEST_CHECK(right(in, 22) == "iusto odio dignissimos");
	TEST_CHECK(right(in, -25) == "et iusto odio dignissimos");
	TEST_CHECK(right(in, 64) == in);

	const std::string eol_linux = "Ingredients\n"
								  "  * 2 cups granulated sugar\n"
								  "  * 1 1/2 cups fresh lemon juice (about 6 to 8 lemons)\n"
								  "  * 5 cups water\n";
	const std::string eol_windows = "Ingredients\r\n"
									"  * 2 cups granulated sugar\r\n"
									"  * 1 1/2 cups fresh lemon juice (about 6 to 8 lemons)\r\n"
									"  * 5 cups water\r\n";

	const std::string eol_src = "Ingredients\r\n"
								"  * 2 cups granulated sugar\n"
								"  * 1 1/2 cups fresh lemon juice (about 6 to 8 lemons)\r\n"
								"  * 5 cups water\n";
	s = eol_src;
	normalize_eol(s);
	TEST_CHECK(s == eol_linux);
	normalize_eol(s, EOLWindows);
	TEST_CHECK(s == eol_windows);

	s = "coffee please!";
	normalize_eol(s);
	TEST_CHECK(s == "coffee please!");
	s = "ice cold tea";
	normalize_eol(s, EOLWindows);
	TEST_CHECK(s == "ice cold tea");

	TEST_CHECK(strip_prefix("epipaleolithic", "epi") == "paleolithic");
	TEST_CHECK(strip_prefix("paleopaleozoic", "paleo") == "paleozoic");
	TEST_CHECK(strip_prefix("neolithic", "meso") == "neolithic");
	TEST_CHECK(strip_prefix("chalcolithic", "") == "chalcolithic");
	TEST_CHECK(strip_prefix("", "paleo") == "");

	TEST_CHECK(strip_suffix("walliserops trifurcatus", "trifurcatus") == "walliserops ");
	TEST_CHECK(strip_suffix("dimetrodon.jpg.jpg", ".jpg") == "dimetrodon.jpg");
	TEST_CHECK(strip_suffix("conolichas angustus", "triconicus") == "conolichas angustus");
	TEST_CHECK(strip_suffix("buenellus higginsi", "") == "buenellus higginsi");
	TEST_CHECK(strip_suffix("", "plicatus") == "");

	const std::string word_wrap_in = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc sagittis orci porta massa iaculis efficitur.";
	const std::string word_wrap_30 = "Lorem ipsum dolor sit amet, consectetur\n"
									 "adipiscing elit. Nunc sagittis\n"
									 "orci porta massa iaculis efficitur.";
	const std::string word_wrap_26_sp4 = "Lorem ipsum dolor sit amet,\n"
										 "    consectetur adipiscing elit.\n"
										 "    Nunc sagittis orci porta massa\n"
										 "    iaculis efficitur.";
	const std::string word_wrap_24 = "Lorem ipsum dolor sit amet,\n"
									 "consectetur\n"
									 "adipiscing elit. Nunc sagittis\n"
									 "orci porta massa iaculis\n"
									 "efficitur.";
	TEST_CHECK(word_wrap(word_wrap_in, 30) == word_wrap_30);
	TEST_CHECK(word_wrap(word_wrap_in, 26, 4, ' ') == word_wrap_26_sp4);
	TEST_CHECK(word_wrap(word_wrap_in, word_wrap_in.size()) == word_wrap_in);
	TEST_CHECK(word_wrap(word_wrap_30, 24) == word_wrap_24);

	const std::string word_min = "Lorem ipsum dolor;sit amet; consectetur adipiscing elit.";
	const std::string word_1 = "Lorem\nipsum\ndolor;\nsit\namet;\n consectetur\nadipiscing\nelit.";
	TEST_CHECK(word_wrap(word_min, -10) == word_1);

	TEST_CHECK(name_to_path("user@home/readme first!") == "user-home-readme-first-");
	TEST_CHECK(name_to_path("random_filename_00") == "random_filename_00");

	TEST_CHECK(pad_left("Heoclisis fulva", 9, '.') == "Heoclisis fulva");
	TEST_CHECK(pad_left("Megaloprepus caerulatus", 28, '#') == "#####Megaloprepus caerulatus");
	TEST_CHECK(pad_left("Acanthacorydalis fruhstorferi", 33) == "    Acanthacorydalis fruhstorferi");
	
	TEST_CHECK(pad_left("Valanga irregularis", 11, '|') == "Valanga irregularis");
	TEST_CHECK(pad_right("Goliathus regius", 23, ':') == "Goliathus regius:::::::");
	TEST_CHECK(pad_right("Macrodontia cervicornis", 27) == "Macrodontia cervicornis    ");
	TEST_CHECK(pad_right("Bocydium globulare", -2) == "Bocydium globulare");
}
