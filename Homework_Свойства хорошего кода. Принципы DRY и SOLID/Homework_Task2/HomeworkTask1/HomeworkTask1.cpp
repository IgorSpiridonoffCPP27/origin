#include <fstream>
#include <iostream>
#include <memory>


class IDataFormatter {
public:
    virtual ~IDataFormatter() = default;
    virtual std::string format(const std::string& text) = 0;
};

class TextFormatter : public IDataFormatter {
public:
    std::string format(const std::string& text) override {
        return text;
    }
};

class HTMLFormatter : public IDataFormatter {
public:
    std::string format(const std::string& text) override {
        return "<html>" + text + "</html>";
    }
};

class JSONFormatter : public IDataFormatter {
public:
    std::string format(const std::string& text) override {
        return "{ \"data\": \"" + text + "\"}";
    }
};

class IDataSaver {
public:
    virtual ~IDataSaver() = default;
    virtual void save(std::ofstream& file, const std::string& text, IDataFormatter& formatter) = 0;
};

class FileSaver : public IDataSaver {
public:
    void save(std::ofstream& file, const std::string& text, IDataFormatter& formatter) override {
        file << formatter.format(text);
    }
};

int main() {
    std::unique_ptr<IDataFormatter> formatter = std::make_unique<HTMLFormatter>();
    std::unique_ptr<IDataSaver> saver = std::make_unique<FileSaver>();

    std::ofstream file("output.html");
    saver->save(file, "Hello World!", *formatter);

    return 0;
}