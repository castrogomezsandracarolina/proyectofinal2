#include "Catalogo.h"
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <filesystem>
#include <exception>
#include <vector>

class Ventana final : public QMainWindow, public Observador {
    enum class Vista { Inicio, Buscar, Favoritos, Pendientes, Historial, Vistas, Recomendaciones };
    Vista vista = Vista::Inicio;
    QLineEdit* entrada = nullptr;
    QComboBox *campo = nullptr, *motor = nullptr;
    QStackedWidget* paginas = nullptr;
    QListWidget *lista = nullptr, *inicioPendientes = nullptr, *inicioRecomendadas = nullptr;
    QTextBrowser* detalle = nullptr;
    QLabel *contador = nullptr, *paginaActual = nullptr, *tituloResultados = nullptr, *estado = nullptr;
    QPushButton *favorito = nullptr, *pendiente = nullptr, *vistaBoton = nullptr;
    std::vector<int> resultados;
    int seleccionada = 0;
    std::size_t pagina = 0;

    static QString q(const std::string& valor) { return QString::fromStdString(valor); }
    static QString html(const std::string& valor) { return q(valor).toHtmlEscaped(); }
    static QString etiquetaPelicula(const Pelicula& p) {
        return q(p.titulo) + "  •  " + q(p.año) + "\n" + q(p.genero);
    }
    static CampoBusqueda campoElegido(int indice) {
        switch (indice) {
            case 1: return CampoBusqueda::Director;
            case 2: return CampoBusqueda::Elenco;
            case 3: return CampoBusqueda::Genero;
            case 4: return CampoBusqueda::Origen;
            default: return CampoBusqueda::Texto;
        }
    }
    static ModoBusqueda motorElegido(int indice) {
        return indice == 1 ? ModoBusqueda::Lineal : indice == 2 ? ModoBusqueda::Paralela : ModoBusqueda::Trie;
    }
    static QString detectarCSV() {
        QSettings preferencias("CineVerse", "CineVerse");
        const QString guardado = preferencias.value("rutaCSV").toString();
        if (!guardado.isEmpty() && QFileInfo::exists(guardado)) return guardado;
        const QString cwd = QDir::currentPath();
        const QString exe = QCoreApplication::applicationDirPath();
        for (QString raiz : {cwd, exe}) {
            QDir dir(raiz);
            for (int n = 0; n < 4; ++n) {
                for (const QString& nombre : {"data/wiki_movie_plots_deduped.csv", "wiki_movie_plots_deduped.csv"}) {
                    const QString ruta = dir.filePath(nombre);
                    if (QFileInfo::exists(ruta)) return ruta;
                }
                dir.cdUp();
            }
        }
        return {};
    }
    void abrir(const QString& ruta) {
        if (ruta.isEmpty()) return;
        estado->setText("Preparando catalogo e indice de caracteres... Puede tardar unos segundos.");
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        bool ok = false;
        try { ok = Catalogo::instancia().cargar(ruta.toStdString()); }
        catch (const std::exception& e) {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical(this, "Error al cargar CSV", QString::fromUtf8(e.what()));
            estado->setText("Error al cargar los datos.");
            return;
        }
        QApplication::restoreOverrideCursor();
        if (!ok) {
            QMessageBox::warning(this, "CineVerse", "No se pudo abrir el CSV o el perfil. Revisa la ruta y las columnas.");
            estado->setText("No se pudo cargar el catalogo.");
            return;
        }
        QSettings("CineVerse", "CineVerse").setValue("rutaCSV", ruta);
        estado->setText(QString("%1 peliculas · %2 palabras indexadas · %3 nodos de caracteres | Perfil: %4")
                        .arg(static_cast<qulonglong>(Catalogo::instancia().peliculas().size()))
                        .arg(static_cast<qulonglong>(Catalogo::instancia().totalPalabras()))
                        .arg(static_cast<qulonglong>(Catalogo::instancia().totalNodos()))
                        .arg(q(Catalogo::instancia().rutaArchivoPerfil().string())));
        irA(Vista::Inicio);
    }
    void seleccionar(int id) {
        if (!Catalogo::instancia().obtener(id)) return;
        seleccionada = id;
        Catalogo::instancia().registrarVisita(id);  // Cada apertura aparece en Historial.
        mostrarDetalle();
        if (!Catalogo::instancia().perfilGuardadoCorrectamente())
            estado->setText("Aviso: no se pudo guardar el perfil en disco. Revisa los permisos.");
    }
    void llenar(QListWidget* widget, const std::vector<int>& ids, std::size_t maximo) {
        widget->clear();
        auto& c = Catalogo::instancia();
        const std::size_t limite = std::min(ids.size(), maximo);
        for (std::size_t i = 0; i < limite; ++i) {
            const auto* p = c.obtener(ids[i]);
            if (!p) continue;
            auto* item = new QListWidgetItem(etiquetaPelicula(*p));
            item->setData(Qt::UserRole, p->id);
            item->setSizeHint(QSize(260, 62));
            widget->addItem(item);
        }
    }
    void actualizarInicio() {
        auto& c = Catalogo::instancia();
        const auto pend = c.lista(false);
        const auto recom = c.recomendar(5);
        llenar(inicioPendientes, pend, 5);
        llenar(inicioRecomendadas, recom, 5);
    }
    void mostrarResultados() {
        paginas->setCurrentIndex(1);
        lista->clear();
        const auto inicio = pagina * 5;
        const auto fin = std::min(inicio + 5, resultados.size());
        auto& c = Catalogo::instancia();
        for (std::size_t i = inicio; i < fin; ++i) {
            if (const auto* p = c.obtener(resultados[i])) {
                auto* item = new QListWidgetItem(etiquetaPelicula(*p));
                item->setData(Qt::UserRole, p->id);
                item->setSizeHint(QSize(300, 78));
                lista->addItem(item);
            }
        }
        contador->setText(QString("%1 coincidencias | Se muestran de 5 en 5")
                          .arg(static_cast<qulonglong>(resultados.size())));
        paginaActual->setText(QString("Pagina %1 / %2")
            .arg(static_cast<qulonglong>(pagina + 1))
            .arg(static_cast<qulonglong>(std::max<std::size_t>(1, (resultados.size() + 4) / 5))));
    }
    void irA(Vista destino) {
        vista = destino;
        auto& c = Catalogo::instancia();
        if (destino == Vista::Inicio) {
            actualizarInicio();
            paginas->setCurrentIndex(0);
            return;
        }
        if (destino == Vista::Buscar) {
            if (!entrada->text().trimmed().isEmpty()) { buscar(); return; }
            resultados.clear();
            const auto& peliculas = c.peliculas();
            for (const auto& p : peliculas) resultados.push_back(p.id);
        } else if (destino == Vista::Favoritos) resultados = c.lista(true);
        else if (destino == Vista::Pendientes) resultados = c.lista(false);
        else if (destino == Vista::Historial) resultados = c.historial();
        else if (destino == Vista::Vistas) resultados = c.listaVistas();
        else if (destino == Vista::Recomendaciones) resultados = c.recomendar(100);
        switch (destino) {
            case Vista::Favoritos: tituloResultados->setText("Mis Me gusta"); break;
            case Vista::Pendientes: tituloResultados->setText("Ver mas tarde"); break;
            case Vista::Historial: tituloResultados->setText("Historial de aperturas"); break;
            case Vista::Vistas: tituloResultados->setText("Peliculas que ya vi"); break;
            case Vista::Recomendaciones: tituloResultados->setText("Recomendadas para ti"); break;
            default: tituloResultados->setText("Explorar peliculas"); break;
        }
        pagina = 0;
        mostrarResultados();
    }
    void buscar() {
        vista = Vista::Buscar;
        auto& c = Catalogo::instancia();
        const auto texto = entrada->text().trimmed().toStdString();
        const auto metodo = motorElegido(motor->currentIndex());
        const auto filtro = campoElegido(campo->currentIndex());
        if (texto.empty()) {
            resultados.clear();
            for (const auto& p : c.peliculas()) resultados.push_back(p.id);
        } else if (filtro == CampoBusqueda::Texto)
            resultados = c.buscar(texto, metodo);
        else resultados = c.buscarPorTag(texto, filtro, metodo);
        tituloResultados->setText("Resultados de busqueda");
        pagina = 0;
        mostrarResultados();
    }
    void mostrarDetalle() {
        auto& c = Catalogo::instancia();
        const auto* p = c.obtener(seleccionada);
        const bool hay = p != nullptr;
        favorito->setEnabled(hay);
        pendiente->setEnabled(hay);
        vistaBoton->setEnabled(hay);
        if (!p) {
            detalle->setHtml("<h2>Tu proxima pelicula</h2>Selecciona un titulo para leer su sinopsis.");
            return;
        }
        const QString texto = QString(
            "<h2 style='color:#f4f6fa'>%1</h2>"
            "<p style='color:#c5a1af'>%2 &nbsp; · &nbsp; %3</p>"
            "<p><b>Director:</b> %4<br><b>Elenco:</b> %5<br><b>Origen:</b> %6</p>"
            "<h3 style='color:#fa618a'>Sinopsis</h3><p style='line-height:1.5'>%7</p>")
            .arg(html(p->titulo)).arg(html(p->año)).arg(html(p->genero))
            .arg(html(p->director)).arg(html(p->elenco)).arg(html(p->origen))
            .arg(html(p->sinopsis));
        detalle->setHtml(texto);
        favorito->setText(c.esFavorito(p->id) ? "♥ Quitar Me gusta" : "♡ Me gusta");
        pendiente->setText(c.esPendiente(p->id) ? "✓ Quitar de Ver mas tarde" : "+ Ver mas tarde");
        vistaBoton->setText(c.estaVista(p->id) ? "✓ Ya vista (desmarcar)" : "Marcar como vista");
    }
    QPushButton* boton(QVBoxLayout* contenedor, const QString& texto, Vista destino) {
        auto* b = new QPushButton(texto);
        b->setObjectName("nav");
        b->setCursor(Qt::PointingHandCursor);
        contenedor->addWidget(b);
        QObject::connect(b, &QPushButton::clicked, this, [this, destino] { irA(destino); });
        return b;
    }
public:
    Ventana() {
        setWindowTitle("CineVerse | Tu catalogo de cine");
        resize(1280, 780);
        setMinimumSize(1020, 650);
        auto* principal = new QWidget;
        principal->setObjectName("central");
        auto* base = new QVBoxLayout(principal);
        base->setSpacing(14); base->setContentsMargins(22, 16, 22, 16);

        auto* superior = new QHBoxLayout;
        auto* marca = new QLabel("◈  CINEVERSE"); marca->setObjectName("marca");
        superior->addWidget(marca); superior->addStretch();
        auto* cargar = new QPushButton("Abrir archivo CSV");
        cargar->setObjectName("secundario"); superior->addWidget(cargar);
        base->addLayout(superior);
        auto* eslogan = new QLabel("Encuentra historias. Guarda tus favoritas. Descubre que ver despues.");
        eslogan->setObjectName("subtitulo"); base->addWidget(eslogan);
        auto* buscador = new QHBoxLayout;
        entrada = new QLineEdit; entrada->setPlaceholderText("Buscar palabra, frase o subpalabra...");
        buscador->addWidget(entrada, 4);
        campo = new QComboBox;
        campo->addItems({"Titulo / Sinopsis", "Director", "Elenco", "Genero", "Origen"});
        buscador->addWidget(campo, 2);
        motor = new QComboBox;
        motor->addItems({"Trie", "Lineal", "Paralela"});
        buscador->addWidget(motor, 1);
        auto* ejecutar = new QPushButton("Buscar"); buscador->addWidget(ejecutar);
        base->addLayout(buscador);

        auto* cuerpo = new QHBoxLayout;
        cuerpo->setSpacing(16);
        auto* lateral = new QVBoxLayout;
        auto* menu = new QLabel("MI CINEVERSE"); menu->setObjectName("secTitle");
        lateral->addWidget(menu);
        boton(lateral, "Inicio", Vista::Inicio);
        boton(lateral, "Explorar", Vista::Buscar);
        boton(lateral, "♥ Me gusta", Vista::Favoritos);
        boton(lateral, "Ver mas tarde", Vista::Pendientes);
        boton(lateral, "Historial", Vista::Historial);
        boton(lateral, "Ya vistas", Vista::Vistas);
        boton(lateral, "Para ti", Vista::Recomendaciones);
        lateral->addStretch();
        auto* info = new QLabel("Indice de sufijos\n+ 4 patrones\n+ paralelismo");
        info->setObjectName("subtitulo"); lateral->addWidget(info);
        cuerpo->addLayout(lateral, 1);

        paginas = new QStackedWidget;
        auto* inicio = new QWidget;
        auto* columnaInicio = new QVBoxLayout(inicio);
        auto* saludo = new QLabel("Bienvenida a tu cine"); saludo->setObjectName("h1");
        columnaInicio->addWidget(saludo);
        columnaInicio->addWidget(new QLabel("CONTINUA VIENDO · Tu lista Ver mas tarde"));
        inicioPendientes = new QListWidget; columnaInicio->addWidget(inicioPendientes);
        columnaInicio->addWidget(new QLabel("DESCUBRE · Recomendaciones segun tus Me gusta"));
        inicioRecomendadas = new QListWidget; columnaInicio->addWidget(inicioRecomendadas);
        paginas->addWidget(inicio);

        auto* listado = new QWidget;
        auto* columnaResultados = new QVBoxLayout(listado);
        tituloResultados = new QLabel("Explorar peliculas"); tituloResultados->setObjectName("h1");
        columnaResultados->addWidget(tituloResultados);
        contador = new QLabel; contador->setObjectName("subtitulo"); columnaResultados->addWidget(contador);
        lista = new QListWidget; columnaResultados->addWidget(lista);
        auto* paginacion = new QHBoxLayout;
        auto* anterior = new QPushButton("◀ Anterior"); anterior->setObjectName("secundario");
        auto* siguiente = new QPushButton("Siguiente ▶"); siguiente->setObjectName("secundario");
        paginaActual = new QLabel("Pagina 1 / 1");
        paginacion->addWidget(anterior); paginacion->addWidget(paginaActual); paginacion->addWidget(siguiente);
        columnaResultados->addLayout(paginacion);
        paginas->addWidget(listado);
        cuerpo->addWidget(paginas, 5);

        auto* derecha = new QVBoxLayout;
        auto* infoTitulo = new QLabel("DETALLES DE LA PELICULA"); infoTitulo->setObjectName("secTitle");
        derecha->addWidget(infoTitulo);
        detalle = new QTextBrowser; detalle->setOpenExternalLinks(false);
        derecha->addWidget(detalle);
        favorito = new QPushButton("♡ Me gusta");
        pendiente = new QPushButton("+ Ver mas tarde");
        vistaBoton = new QPushButton("Marcar como vista"); vistaBoton->setObjectName("secundario");
        derecha->addWidget(favorito); derecha->addWidget(pendiente); derecha->addWidget(vistaBoton);
        cuerpo->addLayout(derecha, 4);
        base->addLayout(cuerpo, 1);
        estado = new QLabel("Buscando base de datos..."); estado->setObjectName("subtitulo");
        estado->setWordWrap(true); base->addWidget(estado);
        setCentralWidget(principal);
        setStyleSheet(R"CSS(
            QWidget#central, QMainWindow { background-color:#10111b; color:#eef0f7; }
            QWidget { font-family:'Segoe UI'; font-size:13px; color:#eceef5; }
            QLabel#marca { color:#ff527d; font-size:25px; font-weight:900; }
            QLabel#h1 { font-size:23px; font-weight:800; color:#ffffff; padding:7px; }
            QLabel#secTitle { color:#f0a5bb; font-weight:700; padding:9px; }
            QLabel#subtitulo { color:#a8aabe; font-size:12px; }
            QLineEdit, QComboBox, QListWidget, QTextBrowser {
                background:#1b1e2e; color:#f2f3fa; border:1px solid #34394d;
                border-radius:9px; padding:9px; selection-background-color:#ac3157;
            }
            QComboBox QAbstractItemView { background:#1b1e2e; color:white; }
            QListWidget::item { padding:7px; margin-bottom:3px; border-radius:5px; }
            QListWidget::item:selected { background:#442037; border:1px solid #fb517e; }
            QPushButton { background:#ee4774; color:#ffffff; font-weight:700;
                          border:0; border-radius:8px; padding:12px 10px; }
            QPushButton:hover { background:#ff6b92; }
            QPushButton:disabled { background:#4b4d5b; color:#9193a5; }
            QPushButton#nav { background:#1c2030; text-align:left; }
            QPushButton#nav:hover { background:#343047; }
            QPushButton#secundario { background:#303348; }
            QPushButton#secundario:hover { background:#474c66; }
        )CSS");

        QObject::connect(cargar, &QPushButton::clicked, this, [this] {
            const QString archivo = QFileDialog::getOpenFileName(this, "Abrir base de datos", QDir::homePath(), "CSV (*.csv)");
            if (!archivo.isEmpty()) abrir(archivo);
        });
        QObject::connect(ejecutar, &QPushButton::clicked, this, [this] { buscar(); });
        QObject::connect(entrada, &QLineEdit::returnPressed, this, [this] { buscar(); });
        for (QListWidget* widget : {lista, inicioPendientes, inicioRecomendadas}) {
            QObject::connect(widget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
                if (item) seleccionar(item->data(Qt::UserRole).toInt());
            });
        }
        QObject::connect(anterior, &QPushButton::clicked, this, [this] {
            if (pagina > 0) { --pagina; mostrarResultados(); }
        });
        QObject::connect(siguiente, &QPushButton::clicked, this, [this] {
            if ((pagina + 1) * 5 < resultados.size()) { ++pagina; mostrarResultados(); }
        });
        QObject::connect(favorito, &QPushButton::clicked, this, [this] {
            if (seleccionada) Catalogo::instancia().alternarFavorito(seleccionada);
        });
        QObject::connect(pendiente, &QPushButton::clicked, this, [this] {
            if (seleccionada) Catalogo::instancia().alternarPendiente(seleccionada);
        });
        QObject::connect(vistaBoton, &QPushButton::clicked, this, [this] {
            if (seleccionada) Catalogo::instancia().alternarVista(seleccionada);
        });
        Catalogo::instancia().suscribir(this);
        mostrarDetalle();
        QTimer::singleShot(0, this, [this] {
            const QString ruta = detectarCSV();
            if (ruta.isEmpty()) {
                estado->setText("Selecciona Abrir archivo CSV para cargar las peliculas.");
                irA(Vista::Inicio);
            } else abrir(ruta);
        });
    }
    ~Ventana() override { Catalogo::instancia().desuscribir(this); }
    void actualizar() override {
        mostrarDetalle();
        if (vista == Vista::Inicio) actualizarInicio();
        else if (vista != Vista::Buscar) irA(vista); // Biblioteca se actualiza al cambiar un Like.
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    Ventana ventana;
    ventana.show();
    return app.exec();
}
