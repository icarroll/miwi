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

const string WIKI_HEADER = "<html><head></head><body><form method=\"get\" action=\"/edit/$1\"><button type=\"submit\">Edit Page</button></form>\n";
const string WIKI_FOOTER = "\n</body></html>\n";

// turn WikiWords into links, and add html header/footer
string wikify(string raw_text, string wikiword) {
    stringstream raw_text_stream(raw_text);
    Parser markdown = Parser();
    string html_text = markdown.Parse(raw_text_stream);
    regex wikiword_pat("([A-Z][a-z]+[A-Z][a-z]+[A-Za-z]*)");
    string url_fmt = "<a href=\"/wiki/$1\">$1</a>";
    string wiki_text = regex_replace(html_text, wikiword_pat, url_fmt);
    return boost::replace_all_copy(WIKI_HEADER, "$1", wikiword) + wiki_text + WIKI_FOOTER;
}

const string EDIT_HEADER = "<html><head></head><body>Edit page Markdown<form action=\"/save/$1\" method=\"post\"><textarea name=\"text\" rows=\"25\" cols=\"80\" placeholder=\"This page is empty.\">";
const string EDIT_FOOTER = "</textarea><br><input type=\"submit\" value=\"Save Page\"></form></body></html>\n";

// turn html characters into entities, add edit page header/footer
string editify(string raw_text, string wikiword) {
    string edit_text = raw_text;
    boost::replace_all(edit_text, "&", "&amp;");
    boost::replace_all(edit_text, "<", "&lt;");
    boost::replace_all(edit_text, ">", "&gt;");
    return boost::replace_all_copy(EDIT_HEADER, "$1", wikiword)
           + edit_text + EDIT_FOOTER;
}

string postdecode(string raw_text) {
    crow::query_string qs("?" + raw_text);
    return qs.get("text");
}

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){
        crow::response resp;
        resp.redirect("/wiki/HomePage");
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
            resp.redirect("/edit/" + wikiword);
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
        resp.redirect("/wiki/" + wikiword);
        return resp;
    });

    app.port(18080).multithreaded().run();
}
