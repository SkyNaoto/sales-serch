import { Component } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { HttpClientModule } from '@angular/common/http';
import { SearchService, SearchParams, SearchResult } from './services/search.service';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [FormsModule, HttpClientModule],
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css'],
})
export class AppComponent {
  params: SearchParams = {};
  results: SearchResult[] = [];
  total = 0;
  page = 1;
  pageSize = 100;
  totalPages = 0;
  pages: number[] = [];
  loading = false;

  constructor(private searchService: SearchService) {}

  setThisMonth() {
    const now = new Date();
    const firstDay = new Date(now.getFullYear(), now.getMonth(), 1);
    this.params.startDate = this.formatDate(firstDay);
    this.params.endDate = this.formatDate(now);
    // this.doSearch();
  }
  setLastMonth() {
    const now = new Date();
    // 前月の初日を取得
    const firstDayLastMonth = new Date(now.getFullYear(), now.getMonth()-1, 1);
    // 前月の最終日を取得 0は当月の0日目、つまり前月の最終日を指す
    const lastDayLastMonth = new Date(now.getFullYear(), now.getMonth(), 0);
    this.params.startDate = this.formatDate(firstDayLastMonth);
    this.params.endDate = this.formatDate(lastDayLastMonth);
    // this.doSearch();
  }
  setLast30Days(){
    const now = new Date();
    const past = new Date();
    past.setDate(now.getDate() - 30);
    this.params.startDate = this.formatDate(past);
    this.params.endDate = this.formatDate(now);
    // this.doSearch();
  }

  formatDate(date: Date): string {
    // getFullYear()は西暦年を取得する
    const y = date.getFullYear();
    // slice(-2)は、文字列の末尾から2文字を取得する
    // getMonth()は0から始まるため、1を足してから2桁にフォーマットする
    const m = ('0' + (date.getMonth() + 1)).slice(-2);
    // getDate()は日付を取得する
    const d = ('0' + date.getDate()).slice(-2);
    return `${y}-${m}-${d}`;
  }

  get startIndex(): number {
    return (this.page - 1) * this.pageSize + 1;
  }

  get endIndex(): number {
    return Math.min(this.page * this.pageSize, this.total);
  }
  goPage(p: number) {
  this.page = p;
  this.doSearch();
  }
  prevPage() {
    if (this.page > 1) {
      this.page--;
      this.doSearch(false);
    }
  }

  nextPage() {
    if (this.page < this.totalPages) {
      this.page++;
      this.doSearch(false);
    }
  }

  // 検索ボタン用（ページリセット）
  startSearch() {
    this.page = 1;
    this.doSearch(true);
  }

  // 本体検索関数
  doSearch(resetPage = false) {
    this.loading = true;

    if (resetPage) {
      this.page = 1;
    }

    this.params.page = this.page;
    this.params.pageSize = this.pageSize;

    this.searchService.search(this.params).subscribe(res => {
      this.results = res.data;
      this.total = res.total;
      this.page = res.page;
      this.pageSize = res.pageSize;

      this.totalPages = Math.ceil(this.total / this.pageSize);
      this.pages = [];

      const start = Math.max(1, this.page - 3);
      const end = Math.min(this.totalPages, start + 6);

      for (let i = start; i <= end; i++) {
        this.pages.push(i);
      }

      this.loading = false;
    });
  }
}