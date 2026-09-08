#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QTextStream>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <algorithm>
#include <cmath>

struct Chunk {
    QString source;
    QString text;
    QVector<float> embedding;
};

QVector<float> embedText(const QString &text) {
    QVector<float> vector(256, 0.0f);
    const QString normalized = text.toLower().simplified();
    for (int i = 0; i < normalized.size(); ++i) {
        const QString token = normalized.mid(i, qMin(2, normalized.size() - i));
        vector[static_cast<int>(qHash(token) % vector.size())] += 1.0f;
    }
    float norm = 0.0f;
    for (float value : vector) norm += value * value;
    norm = std::sqrt(norm);
    if (norm > 0.0f) for (float &value : vector) value /= norm;
    return vector;
}

float cosineSimilarity(const QVector<float> &a, const QVector<float> &b) {
    float score = 0.0f;
    for (int i = 0; i < qMin(a.size(), b.size()); ++i) score += a[i] * b[i];
    return score;
}

class VectorStore {
public:
    void addDocument(const QString &source, const QString &content) {
        const QStringList parts = content.split(QRegularExpression("\\n\\s*\\n"), Qt::SkipEmptyParts);
        for (const QString &part : parts) {
            const QString text = part.trimmed();
            if (!text.isEmpty()) chunks.append({source, text, embedText(text)});
        }
    }

    QVector<Chunk> search(const QString &query, int topK = 3) const {
        struct Result { float score; Chunk chunk; };
        QVector<Result> results;
        const QVector<float> queryVector = embedText(query);
        for (const Chunk &chunk : chunks)
            results.append({cosineSimilarity(queryVector, chunk.embedding), chunk});
        std::sort(results.begin(), results.end(), [](const Result &a, const Result &b) {
            return a.score > b.score;
        });
        QVector<Chunk> answer;
        for (int i = 0; i < qMin(topK, results.size()); ++i) answer.append(results[i].chunk);
        return answer;
    }

    int size() const { return chunks.size(); }

private:
    QVector<Chunk> chunks;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle("多模态 RAG 知识库");
    window.resize(760, 560);

    auto *layout = new QVBoxLayout(&window);
    auto *title = new QLabel("本地知识库检索演示");
    title->setStyleSheet("font-size: 22px; font-weight: bold;");
    auto *status = new QLabel("尚未导入文档");
    auto *importButton = new QPushButton("导入 TXT / Markdown 文档");
    auto *queryEdit = new QLineEdit;
    queryEdit->setPlaceholderText("输入问题或关键词");
    auto *searchButton = new QPushButton("检索知识库");
    auto *resultView = new QTextBrowser;

    layout->addWidget(title);
    layout->addWidget(status);
    layout->addWidget(importButton);
    layout->addWidget(queryEdit);
    layout->addWidget(searchButton);
    layout->addWidget(resultView, 1);

    VectorStore store;
    QObject::connect(importButton, &QPushButton::clicked, [&] {
        const QStringList files = QFileDialog::getOpenFileNames(&window, "选择知识文件", {}, "文本文件 (*.txt *.md)");
        for (const QString &path : files) {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream.setEncoding(QStringConverter::Utf8);
                store.addDocument(QFileInfo(path).fileName(), stream.readAll());
            }
        }
        status->setText(QString("已建立 %1 个文本块的向量索引").arg(store.size()));
    });

    auto runSearch = [&] {
        if (queryEdit->text().trimmed().isEmpty()) return;
        const QVector<Chunk> matches = store.search(queryEdit->text());
        QString html = QString("<h3>问题：%1</h3>").arg(queryEdit->text().toHtmlEscaped());
        if (matches.isEmpty()) html += "<p>请先导入文档。</p>";
        for (int i = 0; i < matches.size(); ++i) {
            html += QString("<h4>%1. 来源：%2</h4><p>%3</p>")
                        .arg(i + 1)
                        .arg(matches[i].source.toHtmlEscaped())
                        .arg(matches[i].text.toHtmlEscaped().replace("\n", "<br>"));
        }
        resultView->setHtml(html);
    };
    QObject::connect(searchButton, &QPushButton::clicked, runSearch);
    QObject::connect(queryEdit, &QLineEdit::returnPressed, runSearch);

    window.show();
    return app.exec();
}
