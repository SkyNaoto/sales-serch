## まず api_server.ext のビルドの仕方
 MYSY2 MINGW64 を起動し、以下のコマンドでビルドする。
 cd sales-serch/server
 gcc main.c mongoose.c sqlite3.c -o api_server.exe -lws2_32
 これで、api_server.exe が出来上がるので、これを実行する。
## api_server.ext の実行の仕方
 MYSY2 MINGW64 のターミナルで、./api_server.exe を実行
 これでサーバが起動する。なお、同じ場所に、sales.db を配置しておく必要がある。
## sales.db の作成方法
 cd sales-serch/tools
 python create_db.py
 実行時に、得意先マスターNF.csv/売上リストNF_s.csv を同じディレクトリに配置しておく。これらのCSVから sales.db が生成される。
 ※現在は、売上リストNF_s.csvは、100行のデータだけにしているので、本番では全データが入ったファイルに差し替える必要あり。

## Front end の起動
　これは従来通り、ng serve で起動する。





# SalesSerch

This project was generated with [Angular CLI](https://github.com/angular/angular-cli) version 17.3.0.

## Development server

Run `ng serve` for a dev server. Navigate to `http://localhost:4200/`. The application will automatically reload if you change any of the source files.

## Code scaffolding

Run `ng generate component component-name` to generate a new component. You can also use `ng generate directive|pipe|service|class|guard|interface|enum|module`.

## Build

Run `ng build` to build the project. The build artifacts will be stored in the `dist/` directory.

## Running unit tests

Run `ng test` to execute the unit tests via [Karma](https://karma-runner.github.io).

## Running end-to-end tests

Run `ng e2e` to execute the end-to-end tests via a platform of your choice. To use this command, you need to first add a package that implements end-to-end testing capabilities.

## Further help

To get more help on the Angular CLI use `ng help` or go check out the [Angular CLI Overview and Command Reference](https://angular.io/cli) page.
