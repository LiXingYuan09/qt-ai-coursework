#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QWidget>

class LlmClient : public QObject {
    Q_OBJECT
public:
    explicit LlmClient(QObject *parent=nullptr):QObject(parent){}
    void ask(const QJsonArray &messages){
        QNetworkRequest req(QUrl("http://localhost:11434/v1/chat/completions"));
        req.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
        QByteArray key=qgetenv("LLM_API_KEY"); if(!key.isEmpty())req.setRawHeader("Authorization","Bearer "+key);
        QJsonObject body{{"model","qwen2.5:7b"},{"messages",messages},{"stream",true}};
        buffer.clear(); reply=network.post(req,QJsonDocument(body).toJson(QJsonDocument::Compact));
        connect(reply,&QNetworkReply::readyRead,this,&LlmClient::consume);
        connect(reply,&QNetworkReply::finished,this,[this]{
            if(reply->error()!=QNetworkReply::NoError) emit failed(reply->errorString()); else emit done();
            reply->deleteLater(); reply=nullptr;
        });
    }
signals: void delta(const QString &text); void done(); void failed(const QString &text);
private:
    void consume(){
        buffer+=reply->readAll();
        while(true){int end=buffer.indexOf('\n');if(end<0)break;QByteArray line=buffer.left(end).trimmed();buffer.remove(0,end+1);
            if(!line.startsWith("data:"))continue;QByteArray data=line.mid(5).trimmed();if(data=="[DONE]")continue;
            QJsonArray choices=QJsonDocument::fromJson(data).object().value("choices").toArray();
            if(!choices.isEmpty()){QString s=choices[0].toObject().value("delta").toObject().value("content").toString();if(!s.isEmpty())emit delta(s);}
        }
    }
    QNetworkAccessManager network; QNetworkReply *reply=nullptr; QByteArray buffer;
};

int main(int argc,char *argv[]){
    QApplication app(argc,argv); QWidget window;window.setWindowTitle("LinguaChat AI 智能聊天助手");window.resize(760,600);
    QTextBrowser transcript;QLineEdit input;QPushButton send("发送");QVBoxLayout layout(&window);layout.addWidget(&transcript);layout.addWidget(&input);layout.addWidget(&send);
    LlmClient llm;QJsonArray history;
    QObject::connect(&send,&QPushButton::clicked,[&]{QString text=input.text().trimmed();if(text.isEmpty())return;history.append(QJsonObject{{"role","user"},{"content",text}});transcript.append("<b>我：</b> "+text.toHtmlEscaped());transcript.append("<b>AI：</b>");input.clear();llm.ask(history);});
    QObject::connect(&input,&QLineEdit::returnPressed,&send,&QPushButton::click);
    QObject::connect(&llm,&LlmClient::delta,[&](const QString &s){transcript.moveCursor(QTextCursor::End);transcript.insertPlainText(s);});
    window.show();return app.exec();
}
#include "main.moc"
