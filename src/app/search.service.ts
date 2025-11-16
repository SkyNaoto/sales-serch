import { Injectable } from '@angular/core';
import { HttpClient, HttpParams } from '@angular/common/http';
import { Observable } from 'rxjs';

// SearchServiceは、検索APIと通信するためのサービスです。
// 検索条件をパラメータとして受け取り、APIエンドポイントにGETリクエストを送信します。
// 例:
// http://localhost:8081/api/search?customerName=ショーエイ&productCode=1234

// DI(依存性注入)コンテナにSearchServiceを登録 これにより、アプリケーション全体でサービスを利用可能にする。
@Injectable({
  providedIn: 'root'
})
export class SearchService {

  private API_URL = 'http://localhost:8081/api/search';

  // コンストラクタでHttpClientを注入 
  // こうすることで AngularのDIコンテナが、このクラスはHttpClientを必要としていることを認識し、HttpClientを注入してくれる。
  // Angular の DIシステムを利用して、HttpClientのインスタンスを取得
  // Angular では new キーワードを使って直接インスタンス化するのではなく、DIコンテナから提供されるインスタンスを利用する。
  // そのためには、コンストラクタの引数に HttpClient を指定する。
  // なので、constructor に HttpClient を注入するように記載すれば、通常のnewを使ったインスタンス化をしているのと同じ効果が得られる。
  // this.http は HttpClient のインスタンスを指す。
  // このhttpClientもSearchServiceが登録されているDIコンテナから提供される。
  constructor(private http: HttpClient) {}

  // paramsObj: anyの意味は、どのような型のオブジェクトでも受け取れることを示している
  // Observable<any>は、任意の型のデータを非同期に受け取ることができるObservableを返すことを示している
  // そして、ジェネリック型を使用していることになる。
  // ジェネリック型とは,例えば以下のようなコードを例にすると、
  // -------------------------------------------------------
  // function wrap<T>(value: T): T { 
  //   return value; 
  // } 
  // const x = wrap<number>(123); // x は number 型
  // -------------------------------------------------------
  // fnction wrap<T>(value: T): T の T に、
  // wrap<number> と指定して呼び出すと、
  // function wrap<number>(value: number): number として扱われる。
  // つまり、wrap<T> の T を呼び出し時に決めることで、
  // 関数全体の型（引数も戻り値も T）を動的に切り替えられる。
  // wrap<T> の T に “どの型をバインドするか” を呼び出し時に決めることで、関数全体がその型に従って動作する。
  // これがジェネリック型の基本的な考え方であり、型の再利用性と柔軟性を高める手法である。
  search(paramsObj: any): Observable<any> {

    // HttpParamsを使用して空のクエリパラメータを構築
    // HttpParamsはサービスではないので、constructorで注入する必要はない。
    // HttpParamsは不変オブジェクトであり、setメソッドを呼び出すたびに新しいインスタンスが返される。
    // そのため、params変数を再代入して更新する必要がある。
    // 例えば、params = params.set('key', 'value') のように使用する。
    let params = new HttpParams();

    // ObjectはJavaScriptの組み込みオブジェクト（標準クラス）であり、キーと値のペアを保持するための基本的なデータ構造を提供する。
    // Object.keys()メソッドは、指定されたオブジェクトの列挙可能なプロパティ名（キー）を配列として返す。
    // 例えば、const obj = { a: 1, b: 2 }; の場合、Object.keys(obj) は ['a', 'b'] を返す。
    // 具体的には、ここでは例えばparamsObjが { customerName: 'ショーエイ', productCode: '1234' } の場合、
    // Object.keys(paramsObj) は ['customerName', 'productCode'] を返す。
    Object.keys(paramsObj).forEach(key => {
      // paramsObj[key]で各キーに対応する値を取得
      // つまり、paramsObj['customerName'] は 'ショーエイ' を返す。
      const value = paramsObj[key];
      // 値が存在し、空白でない場合にのみクエリパラメータとして追加
      if (value && value.trim() !== '') {
        params = params.set(key, value);
      }
    });
    // HttpClientのgetメソッドを使用してGETリクエストを送信
    // 第一引数にAPIのURLを指定し、第二引数にオプションオブジェクトを渡す。
    // ここでは、paramsプロパティに先ほど構築したHttpParamsオブジェクトを設定している。
    return this.http.get(this.API_URL, { params });
  }
}
