
#include <iostream>
#include <string.h>
using namespace std;


class SimpleText {

public:
    virtual void rendering(const string& text) {
        cout << text;
    }

};

class Decorate : public SimpleText {
    
public:
    SimpleText* __text;
    Decorate(SimpleText* text) : __text(text) {

    }
};

class ItalicText : public Decorate {
    
public:
    ItalicText(SimpleText* text) : Decorate(text) {
        
    }
    virtual void rendering(const string& text) {
        cout << "<i>";
        __text->rendering(text);
        cout << "</i>";

    }
    
};

class BoldText : public Decorate {

public:
    BoldText(SimpleText* text) : Decorate(text) {

    }
    virtual void rendering(const string& text) {
        cout << "<b>";
        __text->rendering(text);
        cout << "</b>";

    }

};
class Paragraph : public Decorate {
public:
    Paragraph(SimpleText* text) : Decorate(text) {

    }
    virtual void rendering(const string& text) {
        cout << "<p>";
        __text->rendering(text);
        cout << "</p>";

    }
};

class Reversed : public Decorate{

public:
    Reversed(SimpleText* text) : Decorate(text) {

    }
    virtual void rendering(const string& text) {
        cout << "<p>";
        string reversed = "";
        for (int i = static_cast<int>(text.length() - 1); i >= 0; i--) {
            reversed += text[i];
        }
        __text->rendering(reversed);
        cout << "</p>";

    }

};

class Link : public Decorate {

public:
    Link(SimpleText* text) : Decorate(text) {

    }
    virtual void rendering(const string& url, const string& text) {
        cout << "<a href="+ url + ">";
        __text->rendering(text);
        cout << "</a>";

    }


};


int main()
{
    SimpleText text;
    ItalicText italtext(&text);
    italtext.rendering("text");

    cout << endl;
    BoldText boldtext(&text);
    boldtext.rendering("bold");

    cout << endl;
    auto t = new ItalicText(new BoldText(new SimpleText));
    t->rendering("end");
    cout << endl;
    auto text_block = new Paragraph(new SimpleText);
    text_block->rendering("Hello world");

    cout << endl;
    auto text_block2 = new Reversed(new SimpleText);
    text_block2->rendering("Hello world");

    cout << endl;
    auto text_block3 = new Link(new SimpleText());
    text_block3->rendering("netology.ru", "Hello world");

}

