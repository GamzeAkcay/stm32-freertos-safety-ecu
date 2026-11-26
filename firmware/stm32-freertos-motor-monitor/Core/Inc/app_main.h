#ifndef APP_MAIN_H
#define APP_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

void app_main(void);   // C kodundan (main.c) çağrılabilen C++ giriş noktası
void app_request_clear(void);   // C kodundan: "arızayı temizlemeyi dene"
#ifdef __cplusplus
}
#endif

#endif
