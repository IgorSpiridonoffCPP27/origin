#include <fstream>

// Нарушение ISP (Interface Segregation Principle)
// Интерфейс слишком "жирный", заставляет реализовывать все форматы,
// даже если они не нужны
class Printable
{
public:
    virtual ~Printable() = default;

    virtual std::string printAsHTML() const = 0;
    virtual std::string printAsJSON() const = 0;
    virtual std::string printAsText() const = 0;
};

// Нарушение SRP (Single Responsibility Principle)
// Класс отвечает и за хранение данных, и за все форматы вывода
// Нарушение OCP (Open-Closed Principle)
// При добавлении нового формата нужно изменять существующий класс
class Data : public Printable
{
public:
    enum class Format  // Нарушение DIP (Dependency Inversion Principle)
    {                 // Зависимость от конкретного перечисления форматов
        kText,
        kHTML,
        kJSON
    };

    Data(std::string data, Format format)
        : data_(std::move(data)), format_(format) {}

    // Нарушение LSP (Liskov Substitution Principle)
    // Методы могут выбрасывать исключения вместо выполнения контракта
    std::string printAsHTML() const override
    {
        if (format_ != Format::kHTML) {
            throw std::runtime_error("Invalid format!");
        }
        return "<html>" + data_ + "<html/>";
    }

    std::string printAsText() const override
    {
        if (format_ != Format::kText) {
            throw std::runtime_error("Invalid format!");
        }
        return data_;
    }

    std::string printAsJSON() const override
    {
        if (format_ != Format::kJSON) {
            throw std::runtime_error("Invalid format!");
        }
        return "{ \"data\": \"" + data_ + "\"}";
    }

private:
    std::string data_;
    Format format_;
};

// Нарушение OCP (Open-Closed Principle)
// При добавлении нового формата нужно изменять эту функцию
void saveTo(std::ofstream& file, const Printable& printable, Data::Format format)
{
    switch (format)  // Нарушение DIP (Dependency Inversion Principle)
    {                // Жёсткая зависимость от конкретных форматов
    case Data::Format::kText:
        file << printable.printAsText();
        break;
    case Data::Format::kJSON:
        file << printable.printAsJSON();
        break;
    case Data::Format::kHTML:
        file << printable.printAsHTML();
        break;
    }
}

// Дублирование кода (не относится к SOLID, но плохая практика)
void saveToAsHTML(std::ofstream& file, const Printable& printable) {
    saveTo(file, printable, Data::Format::kHTML);
}

void saveToAsJSON(std::ofstream& file, const Printable& printable) {
    saveTo(file, printable, Data::Format::kJSON);
}

void saveToAsText(std::ofstream& file, const Printable& printable) {
    saveTo(file, printable, Data::Format::kText);
}