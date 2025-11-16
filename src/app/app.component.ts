import { Component } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';



interface SalesRecord {
  product_name: string;
  product_code: string;
  customer_name: string;
  customer_code: string;
  amount: number;
  quantity: number;
  sold_at: string;
}

@Component({
    selector: 'app-root',
    imports: [CommonModule, FormsModule],
    templateUrl: './app.component.html',
    styleUrls: ['./app.component.css']
})
export class AppComponent {
  productName = '';
  productCode = '';
  customerName = '';
  customerCode = '';

  results: SalesRecord[] = [];
  loading = false;

  async search() {
    this.loading = true;

    const params = new URLSearchParams({
      productName: this.productName,
      productCode: this.productCode,
      customerName: this.customerName,
      customerCode: this.customerCode
    });

    // const res = await fetch(`/api/search?${params.toString()}`);
    const res = await fetch(`http://localhost:8081/api/search?${params.toString()}`);
    this.results = await res.json();
    this.loading = false;
  }
}
