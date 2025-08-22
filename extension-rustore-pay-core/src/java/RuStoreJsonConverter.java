package ru.rustore.defold.core;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import org.json.JSONArray;
import org.json.JSONObject;
import android.util.Log;

public class RuStoreJsonConverter {

    public static final String TAG = "RuStoreJsonConverter";

	public static String convertIdent(String productId) {
		return productId;
	}

    public static String convertProductDetails(String jsonString) {
        try {

			//{
			//"amountLabel":"39.00 руб.",
			//"currency":"RUB",
			//"description":"Товар #00099",
			//"imageUrl":"",
			//"price":3900,
			//"productId":"com.happygames.mergecafe.sku00099",
			//"promoImageUrl":"",
			//"title":"Товар #00099",
			//"type":"CONSUMABLE_PRODUCT"
			//}

			JSONArray jsonArray = new JSONArray(jsonString);
			JSONArray transformedArray = new JSONArray();
			
			for (int i = 0; i < jsonArray.length(); i++) {
				JSONObject original = jsonArray.getJSONObject(i);

				String ident = convertIdent(original.getString("productId"));
				Log.d(TAG, "INFO:RUSTORECORE: convertProductDetails() ident: " + ident);
            	original.put("ident", ident);
				original.put("currency_code", original.get("currency"));
				original.put("price_string", original.get("amountLabel"));

				if (original.has("price")) {
					original.put("price", original.getDouble("price")*.01);
				}

				transformedArray.put(original);
			}

			return transformedArray.toString();

        } catch (Exception e) {
            e.printStackTrace();
            return "[]";
        }
    }

    private static String toISO8601(final Date date) {
        String formatted = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ssZ").format(date);
        return formatted.substring(0, 22) + ":" + formatted.substring(22);
    }

	// enum TransactionState
	// {
	// 	TRANS_STATE_PURCHASING = 0,
	// 	TRANS_STATE_PURCHASED = 1,
	// 	TRANS_STATE_FAILED = 2,
	// 	TRANS_STATE_RESTORED = 3,
	// 	TRANS_STATE_UNVERIFIED = 4,
	// };

	private static int purchaseStateToDefoldState(String purchaseType) {
        int defoldState = 4;
        switch(purchaseType) {
            case "PROCESSING":
                defoldState = 0;
                break;
			case "Failure":
			case "Cancelled":
			case "CANCELLED":
			case "REJECTED":
			case "EXPIRED":
			case "REFUNDED":
			case "REVERSED":
                defoldState = 2;
                break;
            case "PAID":
            case "Success":
            case "CONFIRMED":
                defoldState = 1;
                break;
            default:
                defoldState = 4;
                break;
        }
        return defoldState;
    }

	public static String convertPurchasesDetails(String jsonString) {

		//{"amountLabel":"1 ₽",
		//"currency":"RUB",
		//"description":"Покупка в приложении Рецепт Счастья Product #00001",
		//"developerPayload":"null",
		//"invoiceId":"14601429",
		//"orderId":"9ab251ea-d356-41d8-8f37-78f7ad66b27e",
		//"price":100,"productId":"com.happygames.mergecafe.sku_test",
		//"productType":"CONSUMABLE_PRODUCT",
		//"purchaseId":"05567024-bcfe-47cf-a8b6-4f83926d0c7c",
		//"purchaseTime":"Jul 30, 2025 4:48:42 PM",
		//"purchaseType":"TWO_STEP","quantity":1,"sandbox":false,
		//"status":"CONFIRMED"}

        try {

			JSONArray jsonArray = new JSONArray(jsonString);

			for (int i = 0; i < jsonArray.length(); i++) {
				JSONObject original = jsonArray.getJSONObject(i);

				String ident = convertIdent(original.getString("productId"));
				Log.d(TAG, "INFO:RUSTORECORE: convertPurchasesDetails() ident: " + ident);
				Log.d(TAG, "INFO:RUSTORECORE: convertPurchasesDetails() ident: " + original.toString());

			}
			
			JSONObject original = null;
			for (int i = 0; i < jsonArray.length(); i++) {
				original = jsonArray.getJSONObject(i);

				String status = original.getString("status");
				if(status.equals("INVOICE_CREATED")){
					continue;
				}

				// {
				// "amountLabel":"1 ₽",
				// "currency":"RUB",
				// "description":"Покупка в приложении Рецепт Счастья Product #00001",
				// "developerPayload":"null",
				// "invoiceId":"10000186607",
				// "orderId":"a5486e78-385f-4d8f-90fc-255e9ac9cc47",
				// "price":100,
				// "productId":"com.happygames.mergecafe.sku_test",
				// "productType":"CONSUMABLE_PRODUCT",
				// "purchaseId":"f8e09654-b229-4e5f-8304-bf3b46f87e22",
				// "purchaseTime":"Aug 17, 2025 4:34:36 PM",
				// "purchaseType":"ONE_STEP","quantity":1,"sandbox":false,"status":"CONFIRMED"
				// }

				String ident = convertIdent(original.getString("productId"));
				//Log.d(TAG, "INFO:RUSTORECORE: convertProductDetails() ident: " + ident);

            	original.put("ident", ident);
				original.put("currency_code", original.get("currency"));
				original.put("price_string", original.get("amountLabel"));
				original.put("state", purchaseStateToDefoldState(status));
				original.put("trans_ident", original.get("purchaseId"));
				original.put("receipt", original.get("purchaseId"));
				// original.put("trans_ident", original.get("orderId"));
				// original.put("receipt", original.get("purchaseId"));
				original.put("date", toISO8601(new Date()));
				original.put("original_json", "{}");

				if (original.has("price")) {
					original.put("amount", original.getDouble("price")*.01);
				}
				//return original.toString();
			}

			if(original != null){
				Log.d(TAG, "INFO:RUSTORECORE: convertPurchasesDetails() => " + original.toString());
				return original.toString();
			}

			return "{ \"state\":2, \"date\":\"" + toISO8601(new Date()) + "\", \"ident\":\"\" }";

        } catch (Exception e) {
            e.printStackTrace();
            return "{}";
        }
    }


	public static String getPurchaseProductFailure() {
		try {
			JSONObject original = new JSONObject();
			original.put("ident", "");
			original.put("state", 2);
			original.put("date", toISO8601(new Date()));
			original.put("trans_ident", "");
			String result = original.toString();
			Log.d(TAG, "INFO:RUSTORECORE: getPurchaseProductFailure() => " + result);
			return result;
        } catch (Exception e) {
            e.printStackTrace();
            return "{}";
        }
	}

	public static String convertPurchaseProductFailure(String jsonString, String productId) {

		Log.d(TAG, "INFO:RUSTORECORE: convertPurchaseProductFailure(" + productId + ")");

		//{
		//"simpleName":"ProductPurchaseCancelled",
		//"detailMessage":"Purchase product is cancelled",
		//"purchaseId":"395d7f2b-0df6-4a5a-bd8f-b1ba2c97a88b",
		//"purchaseType":"UNDEFINED",
		//"message":"Purchase product is cancelled",
		//"stackTrace":[],
		//"serialVersionUID":-3042686055658047285
		//}
		
		try {

			JSONObject original = new JSONObject(jsonString);
			String ident = "";
			if(original.has("productId")){
				ident = convertIdent(original.getString("productId"));
			} else {
				ident = convertIdent(productId);
			}

			original.put("ident", ident);
			original.put("state", 2);
			original.put("date", toISO8601(new Date()));
			original.put("trans_ident", "");
			String result = original.toString();
			Log.d(TAG, "INFO:RUSTORECORE: convertPurchaseProductFailure() => " + result);
			return result;

        } catch (Exception e) {
            e.printStackTrace();
            return "{}";
        }
	}

	public static String convertPurchaseDetails(String jsonString, String productId) {

		Log.d(TAG, "INFO:RUSTORECORE: convertPurchaseDetails(" + productId + ")");
		// {
		// "invoiceId":{"value":"10000003105"},
		// "orderId":{"value":"150b990c-3e07-431c-b209-1a9c051d94e0"},
		// "productId":{"value":"com.happygames.mergecafe.sku_test"},
		// "purchaseId":{"value":"d960efaf-6315-4f82-bd8f-7e71c7243c1c"},
		// "purchaseType":"ONE_STEP",
		// "quantity":{"value":1},
		// "sandbox":true
		// }
		
        try {

			JSONObject original = new JSONObject(jsonString);

			//String status = original.getString("status");
			String ident = "";
			if(original.has("productId")){
				JSONObject data = original.getJSONObject("productId");
				ident = convertIdent(data.getString("value"));
			} else {
				ident = convertIdent(productId);
			}

			original.put("ident", ident);
			original.put("state", 1);//purchaseStateToDefoldState(status)
			original.put("date", toISO8601(new Date()));
			// if(original.has("purchaseId")){
			// 	JSONObject data = original.getJSONObject("purchaseId");
			// 	original.put("receipt", data.get("value"));
			// }
			if(original.has("purchaseId")){
				JSONObject data = original.getJSONObject("purchaseId");
				original.put("trans_ident", data.get("value"));
				original.put("receipt", data.get("value"));
			}
			original.put("original_json", jsonString);

			String result = original.toString();
			Log.d(TAG, "INFO:RUSTORECORE: convertPurchaseDetails() => " + result);
			return result;

        } catch (Exception e) {
            e.printStackTrace();
            return "{}";
        }
    }
}
