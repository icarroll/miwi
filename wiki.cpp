#include <iostream>
#include <regex>

#include "crow_all.h"
#include "maddy/parser.h"

using namespace std;
using namespace maddy;

string readfile(string filename) {
    ifstream infile(filename, ios::in);
    if (infile) {
        string data;
        infile.seekg(0, ios::end);
        data.resize(infile.tellg());
        infile.seekg(0, ios::beg);
        infile.read(& data[0], data.size());
        infile.close();
        return data;
    }
    else throw runtime_error("file not found");
}

void writefile(string filename, string text) {
    ofstream outfile(filename, ios::out);
    outfile << text;
    outfile.close();
}

string space_wikiword(string wikiword) {
    regex space_pat("([A-Za-z])([A-Z])(?=[a-z])");
    string spaced_fmt = "$1 $2";
    return regex_replace(wikiword, space_pat, spaced_fmt);
}

// const string WIKI_HEADER = readfile("wiki_header.html");
// const string WIKI_FOOTER = readfile("wiki_footer.html");

// turn WikiWords into links, and add html header/footer
string wikify(string raw_text, string wikiword) {
    boost::replace_all(raw_text, "&", "&amp;");
    boost::replace_all(raw_text, "<", "&lt;");
    boost::replace_all(raw_text, ">", "&gt;");

    stringstream raw_text_stream(raw_text);
    Parser markdown = Parser();
    string html_text = markdown.Parse(raw_text_stream);

    regex wikiword_pat("([A-Z][a-z]+[A-Z][a-z]+[A-Za-z]*)");
    string url_fmt = "<a href=\"/wiki/$1\">$1</a>";
    string wiki_text = regex_replace(html_text, wikiword_pat, url_fmt);

    string spaced_wikiword = space_wikiword(wikiword);

    // string header = boost::replace_all_copy(WIKI_HEADER, "$wikiword", wikiword);
    // boost::replace_all(header, "$spaced_wikiword", spaced_wikiword);

    // return header + wiki_text + WIKI_FOOTER;
    crow::mustache::context ctx;
    ctx["wikiword"] = wikiword;
    ctx["spaced_wikiword"] = spaced_wikiword;
    ctx["text"] = wiki_text;
    return crow::mustache::load("wiki.html").render_string(ctx);
}

// const string EDIT_HEADER = readfile("edit_header.html");
// const string EDIT_FOOTER = readfile("edit_footer.html");

// turn html characters into entities, add edit page header/footer
string editify(string raw_text, string wikiword) {
    string edit_text = raw_text;
    boost::replace_all(edit_text, "&", "&amp;");
    boost::replace_all(edit_text, "<", "&lt;");
    boost::replace_all(edit_text, ">", "&gt;");

    string spaced_wikiword = space_wikiword(wikiword);

    // string header = boost::replace_all_copy(EDIT_HEADER, "$wikiword", wikiword);
    // boost::replace_all(header, "$spaced_wikiword", spaced_wikiword);

    // return header + edit_text + EDIT_FOOTER;
    crow::mustache::context ctx;
    ctx["wikiword"] = wikiword;
    ctx["spaced_wikiword"] = spaced_wikiword;
    ctx["text"] = edit_text;
    return crow::mustache::load("edit.html").render_string(ctx);
}

string postdecode(string raw_text) {
    crow::query_string qs("?" + raw_text);
    return qs.get("text");
}

int main()
{
    crow::SimpleApp app;
    crow::mustache::set_base("templates");

    CROW_ROUTE(app, "/")([](){
        crow::response resp;
        resp.see_other("/wiki/HomePage");
        return resp;
    });

    CROW_ROUTE(app, "/wiki/<string>")([](string wikiword) {
        crow::response resp;
        resp.set_header("Content-Type", "text/html");
        try {
            string raw_text = readfile("pages/" + wikiword);
            string wiki_page = wikify(raw_text, wikiword);
            resp.write(wiki_page);
            return resp;
        }
        catch (const runtime_error & e) {
            resp.see_other("/edit/" + wikiword);
            return resp;
        }
    });

    CROW_ROUTE(app, "/edit/<string>")([](string wikiword) {
        crow::response resp;
        resp.set_header("Content-Type", "text/html");

        string raw_text;
        try {
            raw_text = readfile("pages/" + wikiword);
        }
        catch (const runtime_error & e) {
            raw_text = "";
        }

        string edit_page = editify(raw_text, wikiword);
        resp.write(edit_page);
        return resp;
    });

    CROW_ROUTE(app, "/save/<string>").methods("POST"_method)
            ([](crow::request req, string wikiword) {
        string raw_text = req.body;
        string text = postdecode(raw_text);
        boost::replace_all(text, "\r", "");
        writefile("pages/" + wikiword, text);

        crow::response resp;
        resp.see_other("/wiki/" + wikiword);
        return resp;
    });

    app.port(18080).multithreaded().run();
}
