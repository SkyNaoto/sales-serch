import { ApplicationConfig } from '@angular/core';
import { provideHttpClient } from '@angular/common/http'; // ★ 追加

export const appConfig: ApplicationConfig = {
  providers: [
    provideHttpClient(),  // ★ 追加（最重要！）
  ]
};
