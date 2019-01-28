// MEMEnglish: English words trainer. 
// (c) Makarov Edgar, 2019

//for IO russian text
#ifndef __linux__
	#define _GLIBCXX_USE_CXX11_ABI 0
	#include <io.h>
#endif
#include <fcntl.h>
#include <codecvt>



#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <stdexcept>

//working with time
#pragma warning(disable : 4996)			//enable std::localtime
#include <chrono>
#include <ctime>


class Word {
public:
	std::wstring eng = L"";
	std::wstring rus = L"";
	std::wstring last_date = std::wstring(15, L'\0');
	int run = 0;						//runs in all times
	int err = 0;						//errors in all times
	int err_this = 0;					//errors in this time
	int cnt = 1;						//run times

};

inline std::wstring trim(std::wstring str, std::wstring symbols = L" \t\n\r\f\v\"\'") {
	str.erase(0, str.find_first_not_of(symbols));         //prefixing symbols
	str.erase(str.find_last_not_of(symbols) + 1);         //surfixing symbols
	return str;
}

inline int select_randomly(size_t size) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(0, size - 1);
	int rand_num = dist(gen);
	return rand_num;
}


//open dict from filename
std::vector<Word> openDict(std::wstring dictname) {

	//Read words dictionary from file to dict_raw
	std::vector<std::wstring> dict_raw;

#ifndef __linux__
	std::wifstream fin(("DICT/" + std::string(dictname.begin(), dictname.end()) + ".txt"));
	if (!fin.is_open())
	{
		throw std::runtime_error("Error! Dictionary is not opened!");
	}

	fin.imbue(std::locale(fin.getloc(),				//set file encoding as unicode 
		new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
	while (fin) {
		std::wstring tmp;
		std::getline(fin, tmp);
		if (tmp.find(L'\t') != std::wstring::npos) {
			dict_raw.push_back(tmp);
		}
	}
#else
	// when reading UTF-16 you must use binary mode
	std::ifstream fin(("DICT/" + std::string(dictname.begin(), dictname.end()) + ".txt"), std::ios::binary);
	if (!fin.is_open())
	{
		throw std::runtime_error("Error! Dictionary is not opened!");
	}

	fin.seekg(0, fin.end);
	size_t size = (size_t)fin.tellg();

	//skip BOM
	fin.seekg(2, fin.beg);
	size -= 2;
	
	std::u16string u16((size / 2) + 1, '\0');
	fin.read((char*)&u16[0], size);
	std::wstring tmp(u16.begin(), u16.end());
	
	auto start = tmp.begin();
	for (auto p=tmp.begin(); p != tmp.end(); p++) {
		if ((int) *p == 0x0A) {
			//std::wcout << std::wstring(start, p);
			dict_raw.push_back(std::wstring(start, p));
			start = p;
		}
	}
#endif
	fin.close();


	//Create words from raw dictionary
	std::vector<Word> dict(dict_raw.size());
	for (size_t i = 0; i != dict_raw.size(); i++) {
		int fields = std::count(dict_raw[i].begin(), dict_raw[i].end(), L'\t');
		if ((fields != 1) && (fields != 4)) {
			throw std::runtime_error("Error! Wrong word #" + std::to_string(i + 1));
		}

		size_t tab_pos0, tab_pos1;

		tab_pos0 = 0;
		tab_pos1 = dict_raw[i].find(L'\t', tab_pos0);
		dict[i].eng = trim(dict_raw[i].substr(tab_pos0, tab_pos1 - tab_pos0));

		if (fields != 4) {

			tab_pos0 = tab_pos1 + 1;
			tab_pos1 = dict_raw[i].size();
			dict[i].rus = trim(dict_raw[i].substr(tab_pos0, tab_pos1 - tab_pos0));

		}
		else {

			tab_pos0 = tab_pos1 + 1;
			tab_pos1 = dict_raw[i].find(L'\t', tab_pos0);
			dict[i].rus = trim(dict_raw[i].substr(tab_pos0, tab_pos1 - tab_pos0));

			tab_pos0 = tab_pos1 + 1;
			tab_pos1 = dict_raw[i].find(L'\t', tab_pos0);
			dict[i].last_date = trim(dict_raw[i].substr(tab_pos0, tab_pos1 - tab_pos0));

			tab_pos0 = tab_pos1 + 1;
			tab_pos1 = dict_raw[i].find(L'\t', tab_pos0);
			dict[i].run = std::stoi(trim(dict_raw[i].substr(tab_pos0, tab_pos1 - tab_pos0)));

			tab_pos0 = tab_pos1 + 1;
			tab_pos1 = dict_raw[i].size();
			dict[i].err = std::stoi(trim(dict_raw[i].substr(tab_pos0, tab_pos1 - tab_pos0)));

		}

	}

	return dict;

}


//train words
std::vector<Word> training(std::vector<Word>& dict, std::wstring& dictname,  int train_cnt) {
	std::wcout << L"Ok! There are " << dict.size() << L" words in your dict!\n";


	//hard-mode activation. If "y" then cnt of word increases when error
	std::wcout << L"Activate hard-mode? [y / N] ";
	bool increase_cnt_if_wrong = false;
	std::wstring hardmode;
	std::getline(std::wcin, hardmode);
	hardmode = hardmode.substr(0, 1);
	if ((hardmode == L"y") || (hardmode == L"Y")) {
		increase_cnt_if_wrong = true;
	}

	//initial values of variables
	std::vector<Word> dict_done;
	std::wstring user_input = L"";
	int wordnum = select_randomly(dict.size());
	bool wrong_last_word = false;
	int right_words = 0;
	int wrong_words = 0;
	int counter = 0;
	int points = 0;
	int right_words_in_a_row = 0;
	std::chrono::time_point<std::chrono::system_clock> start_time, curr_time;
	start_time = std::chrono::system_clock::now();
	int mins = 0;
	int secs = 0;
	int num_symbols = 0;

	//main cycle
	while ((dict.size() != 0) && (user_input != L"!exit")) {
		//time
		curr_time = std::chrono::system_clock::now();
		secs = std::chrono::duration_cast<std::chrono::seconds>(curr_time - start_time).count();
		mins = secs / 60;
		secs = secs % 60;

		//statictics line
#ifndef __linux__
		system("cls");
#else
		system("clear");
#endif
		std::wcout << L"Correct: " << right_words << L" ";
		std::wcout << L"Wrong: " << wrong_words << L" ";
		std::wcout << L"Percent: " << (counter > 0 ? ((100 * right_words) / counter) : 0) << L"% ";
		std::wcout << L"Points: " << points << L" ";
		std::wcout << L"Time: " << mins << L"m" << secs << L"s";
		std::wcout << L"\t\tRemain " << dict.size() << L" words.";
		std::wcout << (increase_cnt_if_wrong ? L"\t\tHARD MODE\n" : L"\n");



		//generate word number in dictionary and read answer
		wordnum = wrong_last_word ? wordnum : select_randomly(dict.size());;
		std::wcout << dict[wordnum].rus << L"\n";
		std::getline(std::wcin, user_input);
		user_input = trim(user_input);
		num_symbols += user_input.size();

		if (user_input == L"!next") {
			wrong_last_word = false;
			right_words_in_a_row = 0;
			continue;
		}

		if (user_input == L"!exit") {
			dict[wordnum].run += 1;
			dict[wordnum].err += 1;
		}



		//check if answer is correct
		if (user_input == dict[wordnum].eng) {
			std::wcout << L"Yes!\n";
			dict[wordnum].run += 1;
			dict[wordnum].err += wrong_last_word ? 1 : 0;
			dict[wordnum].cnt -= wrong_last_word ? 0 : 1;
			points += 10 + right_words_in_a_row;
			right_words_in_a_row = wrong_last_word ? 0 : right_words_in_a_row + 1;
			counter += 1;
			right_words += wrong_last_word ? 0 : 1;
			wrong_last_word = false;

			if (dict[wordnum].cnt == 0) {
				dict_done.push_back(dict[wordnum]);
				dict.erase(dict.begin() + wordnum);
			}
		}
		else {
			std::wcout << L"No! Right is: " << dict[wordnum].eng << "\n";
			if (!wrong_last_word) {
				dict[wordnum].err_this += 1;
				dict[wordnum].cnt += increase_cnt_if_wrong ? 1 : 0;
				wrong_words += 1;
			}

			points -= 20;
			points -= increase_cnt_if_wrong ? 2 * right_words_in_a_row : 0;
			right_words_in_a_row = 0;

			wrong_last_word = true;
		}

		std::wstring temp;
		std::getline(std::wcin, temp);
	}

	//create actual date
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::string s(30, '\0');
	std::strftime(&s[0], s.size(), "%d.%m.%Y", std::localtime(&now));
	std::wstring wdate = std::wstring(s.begin(), s.begin() + 10);

	
	//write errors to mistakes_file
	std::sort(dict_done.begin(), dict_done.end(), [](Word &a, Word &b) {return a.err_this > b.err_this; }); //sort dict by errors in this training
	std::wofstream fout("mistakes_" + std::to_string(train_cnt) + ".txt", std::ios_base::binary);
	fout.imbue(std::locale(fout.getloc(),				//set file encoding as UCS-2 LE
		new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));
	unsigned char bom[] = { 0xFF,0xFE };
	fout.write((wchar_t*)bom, 1);

	int wrong_count = 0;
	int wrong_count_in_done = 0;
	int not_used = 0;
	fout << L"Your mistakes from last training " << wdate << L".\r\n";
	fout << L"Wrong Answers\tWord\tTranslation\r\n";
	for (size_t i = 0; i != dict_done.size(); i++) {
		dict_done[i].last_date = wdate;
		if (dict_done[i].err_this != 0) {
			fout << dict_done[i].err_this << L"\t";
			fout << dict_done[i].eng << L"\t";
			fout << dict_done[i].rus << L"\r\n";
			wrong_count++;
			wrong_count_in_done++;
		}
	}
	for (size_t i = 0; i != dict.size(); i++) {
		if (dict[i].err_this != 0) {
			dict[i].last_date = wdate;
			fout << dict[i].err_this << L"\t";
			fout << dict[i].eng << L"\t";
			fout << dict[i].rus << L"\r\n";
			wrong_count++;
		}
		else {
			not_used++;
		}
	}
	fout.close();

	//write statistics of this game on screen
#ifndef __linux__
	system("cls");
#else
	system("clear");
#endif
	int words = dict_done.size() + dict.size();
	std::wcout << L"Congradulations! You remember ";
	std::wcout << (dict_done.size() - wrong_count_in_done) << " ";
	std::wcout << L"of " << (words - not_used) << " words!\t";
	std::wcout << wrong_count << L" is wrong!\n";
	std::wcout << (100 * (dict_done.size() - wrong_count_in_done)) / words << L"% ";
	std::wcout << L"Quality percent and " << points << L" Points (of " << (19 + words) * words / 2 << L")\n";
	std::wcout << L"Your time is " << mins << L"m " << secs << L"s\n";
	std::wcout << L"Your speed is " << floor(num_symbols / (mins + secs*1.0/60)) << L" sym/min\n";




	//write this training to history file
	if (train_cnt == 1) {
#ifndef __linux__
		std::wofstream fout_hist("history.txt", std::ios_base::binary | std::ios_base::app);
		fout_hist.imbue(std::locale(fout.getloc(),				//set file encoding as UCS-2 LE
			new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));
		fout_hist << wdate << L"\t";
		fout_hist << dictname << L"\t";
		fout_hist << words << L"\t";
		fout_hist << points << L"\t";
		fout_hist << (100 * (dict_done.size() - wrong_count_in_done)) / words << L"%\t";
		fout_hist << mins << L"m" << secs << L"s\t";
		fout_hist << floor(num_symbols / (mins + secs * 1.0 / 60)) << L"\t";
		fout_hist << (dict_done.size() - wrong_count_in_done) << L"\t";
		fout_hist << wrong_count << L"\t";
		fout_hist << not_used;
		if (increase_cnt_if_wrong) {
			fout_hist << L"\thardmode";
		}
		fout_hist << L"\r\n";
		fout_hist.close();	
#else
		std::wofstream fout_hist("history_linux.txt", std::ios_base::binary | std::ios_base::app);
		fout_hist << wdate << L"\t";
		fout_hist << dictname << L"\t";
		fout_hist << words << L"\t";
		fout_hist << points << L"\t";
		fout_hist << (100 * (dict_done.size() - wrong_count_in_done)) / words << L"%\t";
		fout_hist << mins << L"m" << secs << L"s\t";
		fout_hist << floor(num_symbols / (mins + secs * 1.0 / 60)) << L"\t";
		fout_hist << (dict_done.size() - wrong_count_in_done) << L"\t";
		fout_hist << wrong_count << L"\t";
		fout_hist << not_used;
		if (increase_cnt_if_wrong) {
			fout_hist << L"\thardmode";
		}
		fout_hist << L"\r\n";
		fout_hist.close();	
#endif
	}
	

	//return dict with done words sorted by errors in all time
	std::sort(dict_done.begin(), dict_done.end(), [](Word &a, Word &b) {double c1 = (a.err * 1.0 / a.run);
																	    double c2 = (b.err * 1.0 / b.run);
																	    return (c1 == c2) ? (a.err > b.err) : (c1 > c2); });
	return dict_done;
}


int main()
{
#ifndef __linux__
		system("cls");
#else
		system("clear");
#endif

	//enable utf-16 in win console
#ifndef __linux__
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stdin), _O_U16TEXT);
#else
	setlocale(LC_ALL, "");
#endif

	//greeting words
	std::wcout << L"Привет! Hello! Nice to see you in MEMEnglish v2.0 !\n";
	std::wcout << L"Makarov Edgar (c) Moscow, 2017-2019\n\n";
	std::wcout << L"Input name of Dictionary file\n";

	//open dictionary
	std::vector<Word> dict;
	std::wstring dictname;
	bool dict_not_opened = true;
	while (dict_not_opened) {
		try {
			std::wcout << L"./DICT/... .txt ?\n";
			std::getline(std::wcin, dictname);
			dict = openDict(dictname);
			dict_not_opened = false;

		}
		catch (std::runtime_error exc) {
			std::wcout << exc.what() << L" Try again!\n\n";
			std::wcout << L"Hint: Only name of name.txt file (UCS-2 LE encoding). Should be in DICT/ folder. Tab is delimeter.\n";
		}
	}

	//Train words
	bool need_train = true;
	int train_cnt = 0;
	while (need_train) {
		//train
		train_cnt++;
		std::vector<Word> dict_done = training(dict, dictname , train_cnt);

#ifndef __linux__
		//update statistics in dict
		if ((train_cnt == 1) && (dict_done.size() != 0)) {

			//write dict with updated statictics
			std::wofstream fout("DICT/" + std::string(dictname.begin(), dictname.end()) + ".txt", std::ios_base::binary);
			fout.imbue(std::locale(fout.getloc(),				//set file encoding as UCS-2 LE
				new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));
			unsigned char bom[] = { 0xFF,0xFE };
			fout.write((wchar_t*)bom, 1);
			
			for (size_t i = 0; i != dict_done.size(); i++) {
				fout << dict_done[i].eng << L"\t";
				fout << dict_done[i].rus << L"\t";
				fout << dict_done[i].last_date << L"\t";
				fout << dict_done[i].run << L"\t";
				fout << dict_done[i].err << L"\r\n";
			}
			for (size_t i = 0; i != dict.size(); i++) {
				if (dict[i].run != 0) {
					fout << dict[i].eng << L"\t";
					fout << dict[i].rus << L"\t";
					fout << dict[i].last_date << L"\t";
					fout << dict[i].run << L"\t";
					fout << dict[i].err << L"\r\n";
				}
				else {
					fout << dict[i].eng << L"\t";
					fout << dict[i].rus << L"\r\n";
				}
			}
			fout.close();

			//write errors to last_mistakes dict
			std::wofstream fout2("DICT/Last_Mistakes.txt", std::ios_base::binary);
			fout2.imbue(std::locale(fout2.getloc(),				//set file encoding as UCS-2 LE
				new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));
			fout.write((wchar_t*)bom, 1);

			for (size_t i = 0; i != dict_done.size(); i++) {
				if (dict_done[i].err_this != 0) {
					fout2 << dict_done[i].eng << L"\t";
					fout2 << dict_done[i].rus << L"\t";
					fout2 << dict_done[i].last_date << L"\t";
					fout2 << dict_done[i].run << L"\t";
					fout2 << dict_done[i].err << L"\r\n";
				}
			}
			for (size_t i = 0; i != dict.size(); i++) {
			for (size_t i = 0; i != dict.size(); i++) {
				if (dict[i].err_this != 0) {
					fout2 << dict[i].eng << L"\t";
					fout2 << dict[i].rus << L"\t";
					fout2 << dict_done[i].last_date << L"\t";
					fout2 << dict[i].run << L"\t";
					fout2 << dict[i].err << L"\r\n";
				}
			}
			fout2.close();
		}
#endif

		//collect error words to new dict
		std::vector<Word> dict_err;
		for (size_t i = 0; i != dict.size(); i++) {
			if (dict[i].err_this != 0) {
				dict[i].err_this = 0;
				dict[i].cnt = 1;
				dict_err.push_back(dict[i]);
			}
		}
		for (size_t i = 0; i != dict_done.size(); i++) {
			if (dict_done[i].err_this != 0) {
				dict_done[i].err_this = 0;
				dict_done[i].cnt = 1;
				dict_err.push_back(dict_done[i]);
			}
		}
		dict = dict_err;

		//if no errors then end
		if (dict.size() == 0) {
			need_train = false;
			std::wstring temp;
			std::getline(std::wcin, temp);
		}
		else {
			//ask about retrain errors
			std::wcout << L"\n" << L"Do you want to remember wrong words? [y / N] ";
			std::wstring retrain;
			std::getline(std::wcin, retrain);
			retrain = retrain.substr(0, 1);
			if ((retrain != L"y") && (retrain != L"Y")) {
				need_train = false;
			}
		}
	}

	//end program
	return 0;
}
