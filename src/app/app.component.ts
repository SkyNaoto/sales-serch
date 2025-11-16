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
  loading = false;

  constructor(private searchService: SearchService) {}

  doSearch() {
    this.loading = true;
    this.searchService.search(this.params).subscribe({
      next: (res: SearchResult[]) => {   // ← 型追加
        this.results = res;
        this.loading = false;
      },
      error: (err: any) => {            // ← 型追加
        console.error(err);
        this.loading = false;
      }
    });
  }

}
