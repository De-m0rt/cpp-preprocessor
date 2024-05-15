#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool Preprocess(const path& in_file, ofstream& dumFile, const vector<path>& include_directories)
{
    ifstream read_file(in_file);
    if (!read_file) { return false; }

    static regex num_reg(R"(\s*#\s*include\s*\"([^\"]*)\"\s*)");
    static regex num_reg1(R"(\s*#\s*include\s*<([^>]*)>\s**)");
    smatch m;
    string y;
    int line = 0;
    while (getline(read_file,y))
    {
        ++line;
        if (regex_match(y,m,num_reg) || regex_match(y,m,num_reg1) )
        {
            bool is_found = false;
            path p = in_file.parent_path() / string(m[1]);
            if (Preprocess(p,dumFile,include_directories))
            {
                is_found = true;
            }
            else
            {
                for (auto include_path: include_directories)
                {
                    if (Preprocess(path(include_path / string(m[1])),dumFile,include_directories))
                    {
                        is_found = true;
                        break;
                    }
                }
            }
            if (is_found == false)
            {
                cout<<"unknown include file "<<p.filename().string()<<" at file "<<in_file.string()<<
            " at line "<<line<<endl;
                return false;
            }
        }
        else dumFile<<y<<'\n';
    }
    return true;
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories)
{
    ifstream read_file(in_file);
    if (!read_file) { return false; }

    ofstream out(out_file, ios::out | ios::app);
    if (!out) { return false; }

    return Preprocess(in_file,out,include_directories);
}



string GetFileContents(string file) {
    ifstream stream(file);
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {

    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());

}

int main() {
    Test();
}
