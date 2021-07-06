// copyright Francois Vogel - all rights reserved
// contact me to ask for permissions (namely, but not limited to copying and using)

#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <set>
#include <cstring>
#include <sstream>
#include <unordered_set>

using namespace std;

string root;

// run execution infinitely
void stop() {
    cout << "EXECUTION IS STOPPED AND CANNOT BE RESTARTED, PLEASE CLOSE AND RESTART THE PROGRAM";
    while (true) {}
}

// to an announcement in cmd
void announce(string s) {
    cout << s << endl;
}

// error reporting protocol in cmd
void reportError(string s) {
    announce("FATAL ERROR: "+s);
    stop();
}

// returns true if file exists, no otherwise
bool exists(string s) {
    try {
        ifstream test(s);
        char c;
        test >> c;
        if (c == 0) return false;
    }
    catch (...) {
        return false;
    }
    return true;
}

// convert ifstream to string
string streamAllContent(ifstream& stream) {
    stringstream ss;
    ss << stream.rdbuf();
    string data(ss.str());
    return data;
}

// fetch header data in files that need to be parsed
map<string, string> fetchHeaderData(ifstream& toParse) {
    announce("starting to fetch header data");
    char f1, f2, f3, f4;
    toParse >> f1 >> f2 >> f3;
    if (f1 != '-' or f2 != '-' or f3 != '-') reportError("first four character of file do not match, they should be ---");
    string dummy;
    getline(toParse, dummy);
    map<string, string> dict;
    while (true) {
        if (toParse.eof()) reportError("eof reached, last three delimiting characters (---) were not found");
        string line;
        getline(toParse, line);
        if (line == "---") break;
        string key = "", attr = "";
        bool doubleDotSeen = false;
        for (int i = 0; i < line.size(); i++) {
            if (line[i] == ':') {
                doubleDotSeen = true;
                i++; // skip white space just after
            }
            else if (doubleDotSeen) attr += line[i];
            else key += line[i];
        }
        if (!doubleDotSeen) reportError("double dot lacking, make sure to have key: attribute pairs");
        if (key.empty() or attr.empty()) reportError("key and/or attribute empty");
        dict[key] = attr;
    }
    announce("header data fetch successful");
    return dict;
}

// separate a string with delimiter
vector<string> separate(string data, char delim) {
    vector<string> vec;
    string ac = "";
    for (char i : data) {
        if (i == ',') {
            if (!ac.empty()) vec.push_back(ac);
            ac = "";
        }
        else if (i != ' ') ac += i;
    }
    if (!ac.empty()) vec.push_back(ac);
    return vec;
}

// get attribute from a key in map
string getAttribute(map<string, string> m, string key) {
    announce("request for attribute of key="+key);
    if (m.find(key) == m.end()) reportError("key not found");
    return m[key];
}

// update the .html of the tag
void updateTagPage(string tag) {
    if (!exists(root+"index/begin.html")) reportError("index/begin.html not found");
    ifstream indexBeginStream(root+"index/begin.html");
    string html = streamAllContent(indexBeginStream);
    if (exists(root+"./metadata.html")) {
        announce("external metadata found in ./metadata.html");
        ifstream metadata(root+"./metadata.html");
        html += streamAllContent(metadata);
        announce("metadata processed successfully");
        metadata.close();
        announce("log0");
    }
    else announce("no external metadata found (optional feature)");
    html += "</head>\n<body>\n";
    announce("log1");
    if (!exists(root+"./header.html")) reportError("no header found");
    ifstream header(root+"./header.html");
    announce("log2");
    html += streamAllContent(header);
    html += "<div id=\"container\">";
    announce("log3");
    // update latest postss
    html += "<h1>Posts with tag <i style=\"color:gray;\">"+tag+"</i>:</h1>\n";
    string tagPath = root+"../byTag/"+tag+"/posts.txt";
    if (!exists(tagPath)) reportError("did not find tagPath");
    ifstream stream(tagPath);
    string data = streamAllContent(stream);
    string prev = "";
    announce("log4");
    html += "<div class=\"blogContainer\">";
    for (char i : data) {
        if (i == ',') {
            if (!prev.empty()) {
                string ls = "";
                int cdn = 4;
                for (char j : prev) {
                    if (cdn <= 0 and j == '<') break;
                    else if(cdn <= 0) ls += j;
                    cdn--;
                }
                html += "<a class=\"blogPost\" href=\"../../"+ls+"/\">"+prev+"</a>";
                prev = "";
            }
        }
        else prev += i;
    }
    html += "</div>";
    html += "</div>";
    // add footer
    if (!exists(root+"./header.html")) reportError("no footer found");
    ifstream footer(root+"./footer.html");
    html += streamAllContent(footer);
    html += "</body>\n</html>";
    ofstream indexHtml(root+"../byTag/"+tag+"/index.html");
    indexHtml << html;
}

// get the html content for the post
string getPostShortContent(map<string, string>& hd) {
    string html = "<h2>"+getAttribute(hd, "title")+"</h2>";
    html += "<h5>By "+getAttribute(hd, "author")+" on "+getAttribute(hd, "day")+"/"+getAttribute(hd, "month")+"/"+getAttribute(hd, "year")+"</h5>";
    html += "<h4>"+getAttribute(hd, "description")+"</h4>";
    return html;
}

// add a blog post to its tag posts.txt file
void tagsAdd(string tag, map<string, string>& headerData) {
    string tagPath = root+"../byTag/"+tag+"/posts.txt";
    if (!exists(tagPath)) reportError("tagPath not found");
    ifstream stream(tagPath);
    string st = streamAllContent(stream);
    unordered_set<string> data;
    string post = getPostShortContent(headerData);
    data.insert(post);
    string prev = "";
    for (char i : st) {
        if (i == ',') {
            if (!prev.empty()) data.insert(prev);
            prev = "";
        }
        else prev += i;
    }
    stream.close();
    ofstream wr(tagPath);
    for (string i : data) wr << i << ",";
    if (data.empty()) wr << ",";
    wr.close();
    updateTagPage(tag);
}

// generate the beginning of the article (title, author, date, etc.) in html format
string generateHtmlInfo(map<string, string>& headerData) {
    string content = "<h1>"+getAttribute(headerData, "title")+"</h1>\n<p class=\"info\">By "+getAttribute(headerData, "author")+" on "+getAttribute(headerData, "day")+"/"+getAttribute(headerData, "month")+"/"+getAttribute(headerData, "year")+" at "+getAttribute(headerData, "hour")+":"+getAttribute(headerData, "minute")+"\n<p class=\"tagInfo\">Tag(s):</p>\n<div class=\"tagContainer\">";
    vector<string> tags = separate(getAttribute(headerData, "tags"), ',');
    for (string i : tags) {
        content += "<a href=\"../byTag/"+i+"/"+"\" class=\"tagElement\">"+i+"</a>";
        tagsAdd(i, headerData);
    }
    content += "</div>";
    return content;
}

// generate Html code for html article
string generateHtml(ifstream& toParse, map<string, string>& headerData) {
    announce("html generation started");
    string html = "<!DOCTYPE html>\n<html lang=\""+getAttribute(headerData, "language")+"\">\n";
    html += "<title>"+getAttribute(headerData, "title")+"</title>\n";
    html += "<head>\n<meta name=\"author\""+getAttribute(headerData, "author")+"\">\n";
    html += "<meta name=\"description\""+getAttribute(headerData, "description")+"\">\n";
    html += "<meta name=\"keywords\""+getAttribute(headerData, "keywords")+"\">\n";
    if (exists(root+"./metadata.html")) {
        announce("external metadata found in ./metadata.html");
        ifstream metadata(root+"./metadata.html");
        html += streamAllContent(metadata);
        announce("metadata processed successfully");
    }
    else announce("no external metadata found (optional feature)");
    announce("current_status= "+html);
    // starting to build body
    html += "</head>\n<body>\n";
    if (!exists(root+"./header.html")) reportError("no header found");
    ifstream header(root+"./header.html");
    html += streamAllContent(header);
    html += "<div id=\"container\">";
    html += generateHtmlInfo(headerData);
    html += streamAllContent(toParse)+'\n';
    html += "</div>";
    if (!exists(root+"./header.html")) reportError("no footer found");
    ifstream footer(root+"./footer.html");
    html += streamAllContent(footer);
    html += "</body>\n</html>";
    announce("html generation completed successfully");
    return html;
}

// parse ./config/posts.txt, TODO: be careful, we can't have two posts written at the exact same time!
map<int, string> parsePosts() {
    map<int, string> m;
    if (!exists(root+"config/posts.txt")) reportError("./config/posts could not be found");
    ifstream data(root+"config/posts.txt");
    string pd;
    getline(data, pd);
    while (!data.eof()) {
        getline(data, pd);
        if (pd.empty()) break;
        string integ = "";
        string pos = "";
        bool part = false;
        for (char i : pd) {
            if (i == ':') {
                part = true;
            }
            else if (part) pos += i;
            else integ += i;
        }
        announce("checking for stoi of "+integ);
        m[stoi(integ)] = pos;
    }
    return m;
}

// update index.html
void updateHtmlIndex() {
    announce("starting index.html update");
    if (!exists(root+"index/begin.html")) reportError("index/begin.html not found");
    ifstream indexBeginStream(root+"index/begin.html");
    string html = streamAllContent(indexBeginStream);
    if (exists(root+"./metadata.html")) {
        announce("external metadata found in ./metadata.html");
        ifstream metadata(root+"./metadata.html");
        html += streamAllContent(metadata);
        announce("metadata processed successfully");
    }
    else announce("no external metadata found (optional feature)");
    html += "</head>\n<body>\n";
    if (!exists(root+"./header.html")) reportError("no header found");
    ifstream header(root+"./header.html");
    html += streamAllContent(header);
    html += "<div id=\"container\">";
    // update latest posts
    html += "<h1>Latest Posts</h1>";
    map<int, string> posts = parsePosts();
    while (posts.size() > 10) posts.erase(prev(posts.end()));
    ofstream postsOut(root+"./config/posts.txt");
    postsOut << "Posts:" << endl;
    html += "<div class=\"blogContainer\">";
    for (pair<int, string> i : posts) {
        postsOut << to_string(i.first) << ":" << i.second << endl;
        announce("---> "+i.second);
        string ls = "";
        int cnt = 0;
        int cdn = 4;
        for (char i : i.second) {
            if (cdn <= 0 and i == '<') break;
            else if(cdn <= 0) ls += i;
            cdn--;
        }
        html += "<a class=\"blogPost\" href=\""+ls+"/\">"+i.second+"</a>";
    }
    html += "</div>";
    // update tags
    if (!exists(root+"config/tags.txt")) reportError("./config/tags.txt not existing");
    ifstream presentTagsStream(root+"config/tags.txt");
    vector<string> alreadyPresentTags = separate(streamAllContent(presentTagsStream), ',');
    html += "<h1>Posts by tags</h1>\n";
    for (string i : alreadyPresentTags) html += "<a href=\"./byTag/"+i+"/"+"\" class=\"tagElement\">"+i+"</a>";
    html += "</div>";
    // add footer
    if (!exists(root+"./header.html")) reportError("no footer found");
    ifstream footer(root+"./footer.html");
    html += streamAllContent(footer);
    html += "</body>\n</html>";
    ofstream indexHtml(root+"../index.html");
    indexHtml << html;
    announce("index.html updated");
}

// clever way to say which post was published before which other one
int encodeTime(map<string, string>& headerData) {
    int comp = stoi(getAttribute(headerData, "year"))*32*12+stoi(getAttribute(headerData, "month"))*32+stoi(getAttribute(headerData, "day"));
    return 100000000-comp;
}

// add a blog post to ./config/posts.txt
void addPost(string addr, int ref) {
    if (!exists(root+"./config/posts.txt")) reportError("./config/posts.txt not found");
    ifstream in(root+"./config/posts.txt");
    string content = streamAllContent(in);
    in.close();
    ofstream out(root+"./config/posts.txt");
    out << content;
    out << ref << ":" << addr;
}

// parse files
void parse(string toParsePath) {
    announce("parsing started");
    if (!exists(toParsePath)) reportError("file to be parsed could not be found (make sure there's no typo in the file name)");
    ifstream toParse(toParsePath);
    map<string, string> headerData = fetchHeaderData(toParse);
    string html = generateHtml(toParse, headerData);
    announce("parsing completed");
    announce("starting to write contents to file");
    string setFilePath = root+"../"+getAttribute(headerData, "title")+"/index.html";
    ofstream setFile(setFilePath);
    setFile << html;
    announce("contents written successfully");
    announce("updating (maybe) tags");
    string path = root+"config/tags.txt";
    if (!exists(path)) reportError("./config/tags/txt not existing");
    ifstream alreadyPresentTagsStream(root+"config/tags.txt");
    vector<string> alreadyPresentTags = separate(streamAllContent(alreadyPresentTagsStream), ',');
    vector<string> tagsVec = separate(getAttribute(headerData, "tags"), ',');
    set<string> tags;
    for (string i : alreadyPresentTags) tags.insert(i);
    for (string i : tagsVec) {
        if (tags.find(i) == tags.end()) {
            announce("new tag found");
            tags.insert(i);
        }
    }
    ofstream rewriteTags(root+"config/tags.txt");
    for (string i : tags) {
        rewriteTags << i;
        if (tags.find(i) != prev(tags.end())) rewriteTags << ',';
    }
    rewriteTags.close(); // changes take effect immediately
    if (tags.empty()) rewriteTags << ','; // to avoid corner case for checking if empty/nonexisting file
    addPost(getPostShortContent(headerData), encodeTime(headerData));
    updateHtmlIndex();
    announce("success for \""+toParsePath+"\"");
}

int main() {
    announce("Please enter the parent directory (with a '/' at the end) of your creator.cpp file (in normal use cases you should just enter \"./\")");
    // ifstream fin("./tasks.in");
    #define fin cin
    // fin >> root;
    fin >> root;
    while (true) {
        announce("Please enter the location of the file you'd like to parse: (e.g. ./toParse.txt) (other special commands feature '0': stop execution)");
        string toParsePath;
        fin >> toParsePath;
        if (toParsePath == "0") {
            announce("requested for termination");
            announce("programm will terminate shortly after running some final tasks");
            break;
        }
        parse(root+"../"+toParsePath+"/"+toParsePath+".txt");
    }
    announce("running final ending tasks");
    updateHtmlIndex();
    if (!exists(root+"config/tags.txt")) reportError("./config/tags.txt not existing");
    ifstream presentTagsStream(root+"config/tags.txt");
    vector<string> alreadyPresentTags = separate(streamAllContent(presentTagsStream), ',');
    for (string i : alreadyPresentTags) updateTagPage(i);
    announce("prog finished");
}