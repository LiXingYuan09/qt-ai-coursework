#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVBoxLayout>
#include <QWidget>

class AuthService {
public:
    AuthService() {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("smart_login.db");
        db.open();
        QSqlQuery q;
        q.exec("CREATE TABLE IF NOT EXISTS users(username TEXT PRIMARY KEY,salt BLOB,digest BLOB)");
        q.exec("CREATE TABLE IF NOT EXISTS login_log(username TEXT,method TEXT,success INTEGER,time TEXT)");
    }
    QByteArray derive(const QString &password,const QByteArray &salt) const {
        QByteArray value=salt+password.toUtf8();
        for(int i=0;i<120000;++i) value=QCryptographicHash::hash(value,QCryptographicHash::Sha256);
        return value;
    }
    bool registerUser(const QString &user,const QString &password,QString &error) {
        if(user.trimmed().isEmpty()||password.size()<8){error="用户名不能为空，密码至少 8 位";return false;}
        QByteArray salt(16,Qt::Uninitialized);
        for(char &c:salt)c=char(QRandomGenerator::global()->bounded(256));
        QSqlQuery q;q.prepare("INSERT INTO users VALUES(?,?,?)");
        q.addBindValue(user.trimmed());q.addBindValue(salt);q.addBindValue(derive(password,salt));
        if(!q.exec()){error="用户名已存在";return false;} return true;
    }
    bool verify(const QString &user,const QString &password,QString &error) {
        QSqlQuery q;q.prepare("SELECT salt,digest FROM users WHERE username=?");q.addBindValue(user.trimmed());
        bool ok=q.exec()&&q.next()&&derive(password,q.value(0).toByteArray())==q.value(1).toByteArray();
        if(!ok)error="用户名或密码错误";
        QSqlQuery log;log.prepare("INSERT INTO login_log VALUES(?,?,?,?)");
        log.addBindValue(user);log.addBindValue("password");log.addBindValue(ok);log.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));log.exec();
        return ok;
    }
private: QSqlDatabase db;
};

int main(int argc,char *argv[]){
    QApplication app(argc,argv); QWidget window; window.setWindowTitle("SecureID 智能认证中心"); window.resize(520,360);
    AuthService auth; auto *user=new QLineEdit; auto *password=new QLineEdit; password->setEchoMode(QLineEdit::Password);
    auto *status=new QLabel("请输入账号与密码"); auto *login=new QPushButton("安全登录"); auto *reg=new QPushButton("注册用户");
    auto *form=new QFormLayout; form->addRow("用户名",user); form->addRow("密码",password);
    auto *layout=new QVBoxLayout(&window); layout->addWidget(new QLabel("<h2>智能登录认证系统</h2>"));layout->addLayout(form);layout->addWidget(login);layout->addWidget(reg);layout->addWidget(status);
    QObject::connect(login,&QPushButton::clicked,[&]{QString e;status->setText(auth.verify(user->text(),password->text(),e)?"认证成功":"认证失败："+e);});
    QObject::connect(reg,&QPushButton::clicked,[&]{QString e;status->setText(auth.registerUser(user->text(),password->text(),e)?"注册成功":"注册失败："+e);});
    window.show(); return app.exec();
}
