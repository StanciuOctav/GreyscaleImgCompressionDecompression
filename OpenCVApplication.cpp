#include "stdafx.h"
#include "common.h"
#include <algorithm>
#include <vector>
#include <set>
#include <fstream>

using namespace cv;
using namespace std;

// Crearea structurii pentru fiecare pixel unde sunt memorate intensitatea, frecventa, codul huffman, copiii
struct Pixel {
    int intensitate;
    int frecventa;
    string cod;
    Pixel* st, * dr;
};

// Crearea histogramei imaginii selectate
vector<int> creare_histograma(Mat_<uchar> img) {
    vector<int> h(256, 0);

    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            h[img(i, j)]++;
        }
    }
    return h;
}

// Dupa calcularea histogramei, se creeaza fiecare pixel unde copii sunt NULL
vector<Pixel> creare_pixeli(vector<int> h) {
    vector<Pixel> pixeli;
    for (int i = 0; i < h.size(); i++) {
        // Pentru fiecare pixel ce se gaseste in imagine o sa fie adaugat in arbore
        if (h[i] != 0) {
            Pixel p;
            p.frecventa = h[i];
            p.intensitate = i;
            p.cod = "";
            p.st = NULL;
            p.dr = NULL;
            pixeli.push_back(p);
        }
    }
    return pixeli;
}

// Crearea unui pixel parinte ce are frecventa suma dintre frecventele copiilor, iar intensitatea lui este -1
Pixel creare_pixel_parinte(int copil1, int copil2) {
    Pixel p;

    p.frecventa = copil1 + copil2;
    p.intensitate = -1;
    p.cod = "";
    p.st = NULL;
    p.dr = NULL;

    return p;
}

// Functie de sortare a unui vector de Pixel in functie de frecventa folosind metoda celor 2 for-uri
vector<Pixel> sortare(vector<Pixel> arr)
{
    for (int i = 0; i < arr.size() - 1; i++) {
        for (int j = i + 1; j < arr.size(); j++) {
            if (arr[i].frecventa > arr[j].frecventa) {
                Pixel aux;
                aux.frecventa = arr[i].frecventa;
                aux.intensitate = arr[i].intensitate;
                arr[i].frecventa = arr[j].frecventa;
                arr[i].intensitate = arr[j].intensitate;
                arr[j].frecventa = aux.frecventa;
                arr[j].intensitate = aux.intensitate;
            }
        }
    }

    return arr;
}

// Crearea vectorului pentru arborele huffman unde se creeaza parintii
vector<Pixel> creare_pixeli_cu_parinti(vector<Pixel> pixeli) {

    vector<Pixel> pixeliCuParinti;
    int ultimele2 = -1;
    for (int i = 0; i < pixeli.size() - 1; i++) {
        // Se iau nodurile cu frecventele cele mai mici (pot fi si parinti luati in considerare)
        pixeliCuParinti.push_back(pixeli[i]);
        pixeliCuParinti.push_back(pixeli[i + 1]);
        ultimele2 += 2;
        // Se creeaza noul nod parinte
        Pixel p = creare_pixel_parinte(pixeliCuParinti[ultimele2 -1].frecventa, pixeliCuParinti[ultimele2].frecventa);
        pixeli[i + 1] = p;
        // Se sorteaza din nou vectorul de pixeli dupa adaugarea noului parinte
        pixeli = sortare(pixeli);
    }
    pixeliCuParinti.push_back(pixeli.back());

    return pixeliCuParinti;
}

// Functie de creare a codului huffman unde codul parintelui este cel al copilului concatenat cu un 1 (dreapta) sau 0 (stanga)
void creare_cod_huffman(string* copil, string parinte, string zeroOrOne) {
    for (auto i : parinte) {
        *copil += i;
    }
    *copil += zeroOrOne;
}

// Functie de parcurgere a arborelui pentru crearea codului nodurilor
void traversare_arbore(Pixel* radacina) {
    if (radacina == NULL)
        return;
    if (radacina->st != NULL) {
        creare_cod_huffman(&radacina->st->cod, radacina->cod, "0");
        traversare_arbore(radacina->st);
    }
    if (radacina->dr != NULL) {
        creare_cod_huffman(&radacina->dr->cod, radacina->cod, "1");
        traversare_arbore(radacina->dr);
    }
}

// Functie de creare a arborelui huffman
vector<Pixel> creare_arbore_huffman(vector<Pixel> pixeli) {

    for (int i = 0; i < pixeli.size() - 2; i += 2) {
        // Se calculeaza frecventa perechilor de noduri din vectorul de pixeli
        int freq = pixeli[i].frecventa + pixeli[i + 1].frecventa;
        // Se parcurge vectorul pentru a se gasi un posibil parinte
        for (int j = i + 2; j < pixeli.size(); j++) {
            if (pixeli[j].frecventa == freq && pixeli[j].intensitate == -1) {
                // Daca se gaseste un parinte cu aceeasi frecventa si care nu are copii atunci copilul stanga va fi primul nod
                // si al copilul dreapta o sa fie al doilea nod si dam break pentru a nu cauta un alt parinte
                if (pixeli[j].st == NULL && pixeli[j].dr == NULL) {
                    pixeli[j].st = &pixeli[i];
                    pixeli[j].dr = &pixeli[i + 1];
                    break;
                }
            }
        }
    }

    // Se apeleaza functia de traversare a arborelui pentru crearea codului huffman pentru fiecare nod
    traversare_arbore(&pixeli.back());

    return pixeli;
}

// Functie de comprimare a imaginii
void comprimare(Mat_<uchar> img, vector<Pixel> pixeli) {
    FILE* f = fopen("compressed_image.bin", "wb");

    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            // Pentru fiecare pixel din imagine se parcurge vectorul de pixeli pentru a se putea scrie codul huffman al acestuia
            // in fisierul binar
            for (auto currPixel : pixeli) {
                if (img(i, j) == currPixel.intensitate) {
                    for (auto c : currPixel.cod) {
                        fwrite(&c, sizeof(char), 1, f);
                    }
                    // aici se scrie '\0' la final
                    fwrite(&currPixel.cod[currPixel.cod.size()], sizeof(char), 1, f);
                }
            }
        }
    }
    fclose(f);
}

// Functie de decomprimare a imaginii
void decomprimare(Mat_<uchar> img, vector<Pixel> pixeli) {
    FILE* f = fopen("compressed_image.bin", "rb");
    Mat_<uchar> uncompImg(img.rows, img.cols);

    char c;
    string code;
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            fread(&c, sizeof(char), 1, f);
            code += c;
            while (c != '\0') {
                fread(&c, sizeof(char), 1, f);
                if (c != '\0')
                    code += c;
            }
            // Se parcurge vectorul de pixeli si daca un pixel are acelasi cod cu cel citit atunci imaginea rezultata primeste
            // acel pixel
            for (auto currPixel : pixeli) {
                if (currPixel.cod.compare(code) == 0) {
                    uncompImg(i, j) = currPixel.intensitate;
                }
            }
            code.erase(0);
        }
    }
    fclose(f);

    imshow("Origianl image", img);
    imshow("Uncompressed Image", uncompImg);
    waitKey(0);
}

// Functia care calculeaza compression_ratio = numarul total de pixeli din imagine impartit la lungimea codului huffman al imaginii
void calculare_raport_compresie(Mat_<uchar> img, vector<Pixel> pixeli) {
    long size = 0;
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            for (auto currPixel : pixeli) {
                if (currPixel.intensitate == img(i, j)) {
                    size += currPixel.cod.size();
                }
            }
        }
    }
    long totalPixels = img.rows * img.cols;
    cout << "Compression ratio = " << (float)totalPixels / size << endl;
}

int main()
{
    Mat_<uchar> img = imread("Images/cameraman.bmp", 0);
    vector<int> h = creare_histograma(img);

    vector<Pixel> pixeli = creare_pixeli(h);
    pixeli = sortare(pixeli);

    vector<Pixel> pixelsWithParents = creare_pixeli_cu_parinti(pixeli);
    pixelsWithParents = creare_arbore_huffman(pixelsWithParents);

    calculare_raport_compresie(img, pixelsWithParents);

    comprimare(img, pixelsWithParents);
    decomprimare(img, pixelsWithParents);

    return 0;
}