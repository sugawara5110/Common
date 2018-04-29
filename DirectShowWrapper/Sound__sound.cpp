//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　　　　  Sound_クラス                                        **//
//**                                 sound関数                                           **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Sound_.h"

Sound_::Sound_(char *fname) {

	BSTR bstr;
	BSTR_Convert(fname, &bstr);

	// Graphを生成
	pMediaControl->RenderFile(bstr);
	SysFreeString(bstr);//bstr解放

	pVideoWindow->put_AutoShow(OAFALSE);  //自動表示しないようにする

	// 再生開始
	pMediaControl->Run();

	pBasicAudio->put_Volume(-10000);//音量調整(-10000 ～ 0)

	//ストリームの時間幅を取得(最初に1回取得すればok)
	pMediaPosition->get_Duration(&time2);

	if (fileDelF)remove(fname);//削除フラグOn:ファイル削除。解放されると削除されてる
}

void Sound_::sound(bool repeat, long volume){

	pBasicAudio->put_Volume(volume);//音声ON

	if (repeat == FALSE){
		pMediaPosition->put_CurrentPosition(0);//再生位置をスタート位置にセット
	}
	else{
		//ストリームの合計時間幅を基準とする、現在の位置を取得する。
		pMediaPosition->get_CurrentPosition(&time1);

		//現位置と終了位置が同じ場合スタート位置にセット
		if (time1 == time2)pMediaPosition->put_CurrentPosition(0);
	}
}

void Sound_::soundoff(){
	pBasicAudio->put_Volume(-10000);//音声OFF
}

void Sound_::soundloop(bool repeat, long volume, REFTIME start, REFTIME end){

	REFTIME s = time2 * start / 100.0;
	REFTIME e = time2 * end / 100.0;

	pBasicAudio->put_Volume(volume);//音声ON

	if (repeat == FALSE){
		pMediaPosition->put_CurrentPosition(s);//再生位置をスタート位置にセット
	}
	else{
		//ストリームの合計時間幅を基準とする、現在の位置を取得する。
		pMediaPosition->get_CurrentPosition(&time1);

		//現位置と終了位置が同じ場合スタート位置にセット
		if (time1 >= e)pMediaPosition->put_CurrentPosition(s);
	}
}