//Night Vision Digital Image Processing
//Necdet Emre Bayraktar

//Gorgon Kütüphaneleri aynen
#include <Gorgon/Main.h>
#include <Gorgon/UI.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/UI/Organizers/Flow.h>
#include <Gorgon/UI/Dialog.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Widgets/Panel.h>
#include <Gorgon/Widgets/Layerbox.h>
#include <Gorgon/Widgets/Button.h>
#include <Gorgon/Widgets/GeometryBoxes.h>
#include <Gorgon/Widgets/Inputbox.h>
#include <array>
#include <cmath>//ekstra
#include <cstdlib>//ekstra
#include <iostream>

namespace UI = Gorgon::UI;
namespace Widgets = Gorgon::Widgets;
namespace Color = Gorgon::Graphics::Color;

using namespace Gorgon::UI::literals;

//Başta bir clamping ile 8 bite sınırlama olacak
//8bit pixel dışına taşmaması için (0-255 aralığı)
int clamping(int v){
    if (v<0) 
    return 0;
    if (v>255)
    return 255;
    return v;
}

//Grayscale dönüşümü
//L=(0.2126)*R+(0.7152)*G+0.0722*(B) 5.pdf sonu
void convertgrayscale(Gorgon::Graphics::Bitmap& img) {
    int w=img.GetSize().Width;
    int h=img.GetSize().Height;
    // notlardan direk grayscale dönüşümünü aldım
    for (int y=0;y<h;y++){
        for (int x=0;x<w;x++) {
            int r=img(x,y,0);
            int g=img(x,y,1);
            int b=img(x,y,2);
            int l=(int)(0.2126*r+0.7152*g+0.0722*b);
            l=clamping(l);//taşma için bir adet daha clamping
            img(x,y,0)=l;
            img(x,y,1)=l;
            img(x,y,2)=l;
        }
    }
}

// histogram->pdf->cdf->replacement table mantığı
// karanlık görüntüde detayları 0-255 aralığına yayar ki bu durum projem için en mühim partlardan biri
void histogramequalization(Gorgon::Graphics::Bitmap& img) {
    int w=img.GetSize().Width;
    int h=img.GetSize().Height;
    int total=w*h;
    int histogram[256]={};
    for (int y=0;y<h;y++)
        for (int x=0;x<w;x++)
            histogram[img(x,y,0)]++;
    int replacement[256];
    float cdf=0.0f;
    for (int i=0;i<256;i++) {
        cdf+=(float)histogram[i]/total;
        replacement[i]=(int)(cdf*255);
    }

    for (int y=0;y<h;y++) {
        for (int x=0;x<w;x++) {
            int v=replacement[img(x,y,0)];
            img(x,y,0)=v;
            img(x,y,1)=v;
            img(x,y,2)=v;
        }
    }
}
//projenin talimat kısmında gaussianblur istendiği için eklendi notlardan ve araştırmalardan ilerleyerek ekledim 3x3 klasik kernel tercih ettim 
 void gaussianblur(Gorgon::Graphics::Bitmap& img){
    //image boyutları alınıp looplarda kullanılacak
int w=img.GetSize().Width;
int h=img.GetSize().Height;
    //ilk bitmape temp ile yazıp sonra img'ye yeniden kopyalanması gerekiyor.Diğer türlü bozunma oldu görüntüde
Gorgon::Graphics::Bitmap temp;
temp.Resize(w,h,Gorgon::Graphics::ColorMode::RGBA);//bu kısım özellikle çok vaktimi yedi doğru çalışma prensibini araştırdım sürekli 
    //gauss kerneli klasik orta 4 yanlar 2 en köşeler 1 olacak şekilde 3x3 lük bir kernel
int kernel[3][3]={
        {1,2,1},
        {2,4,2},
        {1,2,1}};
int sumofthekernel=16;//değerlerin toplamı
for(int y=0;y<h;y++){//tüm pixelleri ziyaret edecek for loopu 
for(int x=0;x<w;x++){
int sum=0;//her pixel için 0 dan başlata başlata ilerleyeceğiz.
for(int ky=-1;ky<=1;ky++){
for(int kx=-1;kx<=1;kx++){
int sx=x+kx,sy=y+ky;//komşu pixelin kordinatları
            //taşan değerler için clamping yapılıp en yakın pixeli seçecek.
if (sx<0) sx=0;
if (sx>=w) sx=w-1;// 4. pdfteki gibi closest pixel seçimi yapılacak.koda dökerken biraz zorlandım sürekli kontrol ve araştırma yaparak yazdım genel olarak gauss kısmı.
if (sy<0) sy=0;
if (sy>=h) sy=h-1;
            sum+=img(sx,sy,0)*kernel[ky+1][kx+1]; //komşu pixeller ve o pixelin kerneli index dönüşümü olarak geçecek. ky-1 0 a 0 1 e 1 2 ye 
            }
        }
int result=sum/sumofthekernel; // ağırlıkların toplamı toplam kernel değerine bölünüp blur değerini çıkacak.
//baştaki temp bitmape yazılacak rgb değerlerine aynı şekilde
temp(x,y,0)=result;//R için
temp(x,y,1)=result;//G için
temp(x,y,2)=result;//B için 
    } 
}
    //tempteki değerleri img'ye geri yazma kodu greenmapvision ve vignetteffect blurlu olanlarla çalışacak 
for(int y=0;y<h;y++){
for(int x=0;x<w;x++){
int v=temp(x,y,0);
img(x,y,0)=v;//R a yazdırma
img(x,y,1)=v;//G 
img(x,y,2)=v;//B 
}
}
}

// baktığım ayarlarla görüşü yemyeşil yapıyor daha gerçekçi nightvision algısı
// R=B=0, G=intensity, A=255 -> gece görüş yeşili
void greenmapvision(Gorgon::Graphics::Bitmap& img) {
    int w=img.GetSize().Width;
    int h=img.GetSize().Height;

    for (int y=0;y<h;y++) {
        for (int x=0;x<w;x++) {
            int intensity=img(x,y,0);
            img(x,y,0)=0;// R=0
            img(x,y,1)=intensity; // G=intensity
            img(x,y,2)=0; // B=0
            img(x,y,3)=255; // A=255 saydamlık kaldırıldı tam yeşil görüntü oldu
        }
    }
}

// vignette efekti karartma ile dürbün gibi
// merkez parlak köşeler koyu factor=1-(dist/maxdist)*strength mantığını araştırıp aldım 
// strength=1.0 -> köşelerde G channel %100 azalacak (değiştirilebilir)
void vignetteffect(Gorgon::Graphics::Bitmap& img) {
    int w=img.GetSize().Width;
    int h=img.GetSize().Height;
    float cx=w/2.0f;
    float cy=h/2.0f;
    float maxdist=std::sqrt(cx*cx+cy*cy);//math kütüphanesi burada gerekiyor.
    float strength=1.0f;//köşe karanlığını buradan değiştirebilirsiniz.

    for (int y=0;y<h;y++) {
        for (int x=0;x<w;x++) {
            float dx=x-cx;
            float dy=y-cy;
            float dist=std::sqrt(dx*dx+dy*dy);// her pixel merkeze uzaklık olarak 
            float factor=1.0f-(dist/maxdist)*strength;
            if (factor<0.0f) factor=0.0f;// güvenlik için bu kısmı da factor kısmı 0 altına düşerse diye ekledim
            int currentG=img(x,y,1);
            int newG=(int)(currentG*factor);
            img(x,y,1)=clamping(newG);
        }
    }
}


// application.h dosyası template'ten alındı ve settings widgetlarından işe yaramayacakları sildim
/**
 * This is your application class. You should modify it as per your needs.
 */
class Application {
public:
    // This function creates some necessary UI parts.
    // Most likely you don't need to change it.
    Application(UI::Window &window);


    // This function is where the example code runs in.
    // You may modify it to keep it like this.
    void Run();

    // This function is called when the user tries to close
    // the window. You may return false to prevent it.
    bool Quit();

    // This function redraws the bitmap on the layer.
    void Redraw();

private:
    Widgets::Panel side;
    UI::Organizers::Flow sideorganizer;
    Widgets::Layerbox layerbox;
    Widgets::Button run, save, zoomin, zoomout;

    Gorgon::Graphics::Layer  targetlayer;
    Gorgon::Graphics::Bitmap bitmap, zoomed;

    static constexpr std::array<int, 8> zoomrates = {1, 2, 3, 4, 6, 8, 12, 16};
    int zoom = 0;

    UI::Window &window;
};


// Application.cpp kısmı templateinden alındı
// buradaki Run kısmı ile size danıştığım UI sorununu çözdüm.

void Application::Run() {
// input.png manuel yerine otomatik olarak rgba ya çevriliyor grayscalede uygulama çalışmıyordu
    system("convert input.png PNG32:input.png 2>/dev/null");
//input girişi belirlenen standartlara uymazsa hata mesajı var
    if (!bitmap.Import("input.png")) {
        UI::ShowMessage("Error", "Couldn't load input.png");
        return;
    }

//yapılan işlemler sırayla çağırılır
    convertgrayscale(bitmap);
    histogramequalization(bitmap);
    gaussianblur(bitmap);
    greenmapvision(bitmap);
    vignetteffect(bitmap);

//son olarak output.png olarak diske kaydedilir eğer hali hazırda aynı dosya varsa üzerine yazılır.
    bitmap.Export("output.png");

    Redraw();//run kısmı ile beraber UI da outputu gösterecek.
}


Application::Application(UI::Window& window) :
    window(window)
{
    window.Add(side);
    side.SetInteriorWidth(8_u);
    side.SetHeight(100_perc);
    side.AttachOrganizer(sideorganizer);

    window.AddNextTo(layerbox);
    layerbox.SetWidth(window.GetInteriorSize().Width - layerbox.GetCurrentLocation().X);
    layerbox.SetHeight(100_perc);

    window.ResizedEvent.Register([this] {
        layerbox.SetWidth(this->window.GetInteriorSize().Width - layerbox.GetCurrentLocation().X);
    });

    layerbox.GetLayer().Add(targetlayer);

    run.Text = "Run";
    window.SetDefault(run);
    run.SetWidth(4);
    run.ClickEvent.Register(*this, &Application::Run);

    save.Text = "Save";
    save.SetWidth(4);
    save.ClickEvent.Register([this] {
        if(!bitmap.HasData()) {
            UI::ShowMessage("Save", "Nothing save, click run");
            return;
        }

        UI::Input<std::string>("Save", "Enter filename", [this](std::string fname) {
            if(fname.find_first_of('.') == fname.npos)
                fname += ".png";

            if(!bitmap.Export(fname)) {
                UI::ShowMessage("Save", "Save failed, check if you can write into the current directory");
            }
        });
    });

    zoomin.Text = "Zoom in";
    zoomin.SetWidth(4);
    zoomin.ClickEvent.Register([this] {
        if(zoom < zoomrates.size() - 1) {
            zoom++;
            Redraw();
        }
    });

    zoomout.Text = "Zoom out";
    zoomout.SetWidth(4);
    zoomout.ClickEvent.Register([this] {
        if(zoom > 0) {
            zoom--;
            Redraw();
        }
    });

    sideorganizer << run << save << zoomin << zoomout;

    Run();
}

bool Application::Quit() {
    return true;
}

void Application::Redraw() {
    targetlayer.Clear();

    if(bitmap.HasData()) {
        auto rate = zoomrates[zoom];
        zoomed = bitmap.ZoomMultiple(rate);

        if(zoom > 4) {
            zoomed.ForAllPixels([rate, this](int x, int y) {
                if(x % rate == 0 || y % rate == 0)
                    zoomed(x, y, 0) = 64 + zoomed(x, y, 0)/2;
            });
        }

        zoomed.Prepare();
        zoomed.Draw(targetlayer, 0,0);
    }
}

constexpr std::array<int, 8> Application::zoomrates;

//main.cpp kısmı aynen sadece isim kısmı değişti 
int main() {
    Gorgon::Initialize("CG");

    //Change this line to change window size or window title
    Gorgon::UI::Window window({1200, 900}, "Night Vision Simulator — Necdet Emre Bayraktar");

    Gorgon::UI::Initialize();

    Application app(window);
    window.AllowResize();

    window.ClosingEvent.Register([&app](bool &allow) {
        allow = app.Quit();
    });

    window.Run();

    return 0;
}
