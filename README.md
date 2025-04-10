### MWM Window Manager
---
<img src="https://i.imgur.com/inMYY3x.png" alt="isolated" width="600"/> 

#### Açıklama
C dilinde yapay zeka destekli olarak yazıldı. Tek bir dosyadan oluşan hafif bir tasarıma -ortalama 1500 satır- sahiptir. Herhangi bir ayar dosyası **bulunmamaktadır.** C dili bilgisi gerektirir. Ana dosya üzerinde gerekli yerlerde **Türkçe** açıklamalara yer verilmiştir. Kullanıcılar kendilerine göre ilgili bölümde değişiklik yapıp kodu tekrar derleyerek kullanabilirler. Diğer C ile yazılmış window managerlarda olduğu gibi -örnek DWM- her değişiklik ardından derlenmelidir.

 - EWMH desteği vardır. (sadece Polybar ile test edildi.)
 - Pencerelerde tiling ve float olarak iki mod vardır.
 - Workspace desteği vardır.
#### Kurulum
```
$ git clone https://github.com/amaranus/mwm
$ cd mwm/
$ make all
$ sudo make install
```
.xinitrc dosyası içerisine aşağıdakini ekleyin;
```
exec mwm
reboot
```
Sistem otomatik başlatma için ayarlanmamışsa tty ekranında login olduktan sonra;

```
startx
```
#### Kullanım
* Alt + 1-9: Workspace değiştirir.
* Alt + Shift + 1-9: Aktif pencereyi belirtilen workspace'e taşır
* Alt + d: dmenu çalıştırır.
* Alt + q: Aktif pencereyi kapatır.
* Alt + t: Tiling/Floating pencere modunu değiştirir.
* Alt + l: Ana bölgeyi %1 genişletir (sağa doğru).
* Alt + h: Ana bölgeyi %1 daraltır (sola doğru).
* Alt + Enter: Ana pencere ile değiştirir.
* Alt + g: Boşlukları aç/kapat.
* Alt + j/k: İç boşlukları azalt/artır.
* Alt + Shift + j/k: Dış boşlukları azalt/artır.
* Alt + Sol/Sağ: Önceki/Sonraki workspace'e geçer.
* Alt + Tab: Workspace içinde pencereler arası geçiş yapar.
* Fare üzerine gelindiğinde ilgili pencere aktif olur veya alt + tab ile aktifleşir. Pencere aktifken fare ile pencerenin dışından sol tuş ile taşınır, sağ tuş ile boyutu ayarlanır.

#### Özet
C ile yazılmış diğer window managerler için bir alternatiftir. Kullanıp geliştirmek isteyenlere ithafen...

---
İletişim: amaranus@hotmail.com
