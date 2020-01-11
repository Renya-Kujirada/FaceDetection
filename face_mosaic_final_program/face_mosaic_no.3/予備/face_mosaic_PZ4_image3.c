#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include "pnmimg.h"

/* 各パラメータの割合の定義 [ ()内はデフォルトの値 ] */
#define AREARATIO 0.001 /* 顔の画像全体の面積に対する下限(0.5%) */
#define AREARATE  0.3   /* 外接四角形における占める割合の下限(30%) */
#define HWMIN     0.8   /* 縦横比の下限(0.8) */
#define HWMAX     1.8   /* 縦横比の上限 (1.8)*/
#define SYMMAX    0.3   /* 左右対称度の上限(0.3) */
#define NOP	  12		/*検出最大人数(10)*/
#define FACE_RATE 1.3		/*顔の縦横比(1.3)*/


/*3つの引数の内、最小値を返す関数*/
double return_min(double a,double b,double c){		
	double min=a;
	if(min>b){
		min=b;
	}
	if(min>c){
		min=c;	
	}
	return min;
}						
/*ここまで*/


/*3つの引数の内、最大値を返す関数*/
double return_max(double a,double b,double c){		
	double max=a;
	if(max<b){
		max=b;
	}
	if(max<c){
		max=c;
	}
	return max;
}		
/*ここまで*/

				
/**********１．肌色画素の検出**********/
/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊肌色画素の検出＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/
void process_no1(RGB_PACKED_IMAGE *input,RGB_PACKED_IMAGE *output){
	int j,k;				//for文カウンター変数
	double R=0,G=0,B=0;			//各画素のRGB値を格納する変数
	int H;					//色相H
	double Y=0.0,Cb=0.0,Cr=0.0;				//Y…輝度,Cb,Cr…色差
		for(k=0;k<input->rows;k++){
			for(j=0;j<input->cols;j++){			//画像中の1つの画素のRBG値を読み取る
				R=(double)input->p[k][j].r;
				G=(double)input->p[k][j].g;
				B=(double)input->p[k][j].b;

				Y = (0.299*R)+(0.587*G)+(0.114*B);	//Y,Cb,Crの計算	
				Cb= (-0.172*R)-(0.399*G)+(0.511*B);
				Cr= (0.511*R)-(0.428*G)-(0.083*B);
		
				R=R/(double)255;			//RGB値を[0.0~1.0]の間で再定義する
				G=G/(double)255;
				B=B/(double)255;		

				//色相Hの計算	
				if(return_max(R,G,B)==R){
					H=(int)(60*(G-B)/(return_max(R,G,B)-return_min(R,G,B)));
				}
				if(return_max(R,G,B)==G){
					H=(int)(60*(B-R)/(return_max(R,G,B)-return_min(R,G,B)))+120;
				}
				else{
					H=(int)(60*(B-G)/(return_max(R,G,B)-return_min(R,G,B)))+240;
				}	
				//ここまで

				//H<0, またはH>360の場合、色相Hの値を0~360の間で再定義			
				if(H<0){
					H=H+360;
				}
				else if(H>360){
					H=H-360;
				}
	
			//ここまで
			//
			//肌色画素の判定:条件[ (H<50||H>210)&&(-50<Cb&&Cb<-20)&&(20<Cr&&Cr<40) ]
			//◀◀条件：(色相<50 または210<色相) かつ (-50<色差<-20) かつ (20<色差<40) かつ (輝度>100)▶▶
				if(H!=-1000){
					if((H<60||H>200)&&(-70<Cb&&Cb<-15)&&(10<Cr&&Cr<30)&&(Y>100)){
						output->p[k][j]=input->p[k][j];
					}
					else{
						output->p[k][j].r=255;			//肌色画素ではないのは白色に変換
						output->p[k][j].g=255;
						output->p[k][j].b=255;
					}
				}	
				R=0;G=0;B=0;
				Y=0;Cb=0;Cr=0;					//使用した変数の初期化
				H=0;		
			}
		}
		printf("＊＊＊\t\t\t肌色画素を抽出しました\t\t\t＊＊＊\n\n");
}

/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊こ　こ　ま　で＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/


/*ラベル番号を修正する関数*/	
void modify_label(int num1,int num2,RGB_PACKED_IMAGE *input, int label[input->rows][input->cols]){	
	int j,k;								//for文カウンター
	for(k=0;k<input->rows;k++){
		for(j=0;j<input->cols;j++){
			if(label[k][j]==num1){			//ラベル番号num1をすべてnum2に置換する
				label[k][j]=num2;
			}
		}
	}
}			
/*ここまで*/


/*4近傍のラベルの最大値を返す関数*/
int search_4neighbors(int y,int x,RGB_PACKED_IMAGE *input,int label[input->rows][input->cols]){		
	int max=0;		//最大値

	if(y-1>=0&&label[y-1][x]>max){				//上のラベルを見る
		max=label[y-1][x];
	}
	if(x-1>=0&&label[y][x-1]>max){				//左のラベルを見る
		max=label[y][x-1];		
	}				
	if(y+1<input->rows&&label[y+1][x]>max){			//下のラベルを見る
		max=label[y+1][x];
	}
	if(x+1<input->cols&&label[y][x+1]>max){			//右のラベルを見る
		max=label[y][x+1];
	}
	return max;
}
/*ここまで*/


/**********２．肌色領域のラベリング**********/
/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊ラベリング関数＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/
int labeling(RGB_PACKED_IMAGE *input,int R,int G,int B,int label[input->rows][input->cols]){
	int j,k,num;						//for文カウンター
	int count=0;						//ラベル最大値
	int total_area;						//領域の数

	for(k=0;k<input->rows;k++){				//ラベルを初期化
		for(j=0;j<input->cols;j++){
			label[k][j]=0;
		}
	}							//ここまで

	for(k=0;k<input->rows;k++){
		for(j=0;j<input->cols;j++){				//位置(j,k)の画素が白でないかつラベル番号が0なら
			if((input->p[k][j].r!=R||input->p[k][j].g!=G||input->p[k][j].b!=B)&&label[k][j]==0){
				num=search_4neighbors(k,j,input,label);	//4近傍のラベルの値をnumに調べる
				if(num==0){	
					label[k][j]=++count;		//ラベル番号が0なら,ラベル番号に1加えて、位置(j,k)のラベル番号をcountにする
				}
				else{
					label[k][j]=num;		//それ以外なら4近傍のいずれかの値を代入
				}
		
			}
		}
	}
	printf("＊＊＊\t\t肌色領域のラベリングを終了しました\t\t＊＊＊\n\n");	//メッセージ出力
	if(count>0){							//ラベル番号が0より大きい(肌色領域が1つ以上存在する)
		for(k=0;k<input->rows;k++){
			for(j=0;j<input->cols;j++){
				if(label[k][j]!=0){			
					num=search_4neighbors(k,j,input,label);	//4近傍のラベル値をもとに戻す
					if(num>label[k][j]){
						modify_label(num,label[k][j],input,label);
					}
				}
			}
		}
		total_area=0;
		for(k=0;k<input->rows;k++){
			for(j=0;j<input->cols;j++){
				if(label[k][j]>total_area){
					total_area++;
					modify_label(label[k][j],total_area,input,label);
				}
			}
		}
		printf("肌色領域の数：\t\t%d個\n",total_area);			//メッセージ出力

		return total_area;				//最終的なラベルの最大値（肌色領域の個数）を返す
	}
	else{
		printf("肌色領域の数：0個\n");				
		return 0;
	}
}


/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊こ　こ　ま　で＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/


/* 画像No.n の(x1,y1)<-->(x2,y2)の左右対称度を返す関数 */
/* 戻り値の範囲は [0, 1] で、0 に近いほど左右対称      */
double symmetry( RGB_PACKED_IMAGE *input, int x1, int y1, int x2, int y2 ){
    int xcenter;    /* 横幅の中点     */
    int xrange;     /* xを動かす範囲  */
    double sum=0.0; /* 色差を表す量   */
    int counter=0;  /* 総画素数       */
    int x,y;    /* 作業変数       */
    double dif;     /* 色差（の二乗） */
    double dr, dg, db, d;       /* 作業変数       */

    xcenter = ( x2 + x1 ) / 2;  /* 横幅の中点 */
    xrange = xcenter - x1;
    /* 画像を折り返して調べます */
    for(x=0;x<xrange;x++){
        for(y=y1;y<=y2;y++){
            counter++;  /* 画素数のカウンターに１を足す */
            dif=0;	/* 初期化 */

            dr = (double)( input->p[y][x1+x].r - input->p[y][x2-x].r );	//	R成分
            if ( dr < 0 ) dr = - dr;
                
            dg = (double)( input->p[y][x1+x].g - input->p[y][x2-x].g );	//	G成分
            if ( dg < 0 ) dg = - dg;
                
            db = (double)( input->p[y][x1+x].b - input->p[y][x2-x].b );	//	B成分
            if ( db < 0 ) db = - db;
                
            d = dr + dg + db;
                
            dif += d / 255.0;
            
            sum += dif / 3.0;
        }
    }
    return sum / counter;
}
/*ここまで*/


/**********３．図形特徴の算出及び顔の領域の選別**********/
/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊ラベル番号ごとの面積計算関数＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/
int cal_area_label(RGB_PACKED_IMAGE *input,int lab[input->rows][input->cols],int n){
	int j,k;		/* 制御変数 */
	int sum_area=0;		/* 領域の面積 */
	int rgbmin[3],rgbmax[3];  /* 範囲を調べる変数 */
	int xmin=0,ymin=0,xmax=0,ymax=0;  /* 領域の左上と右下の座標 */
    	double W,H;       /* 横と縦 */
    	int colrange;     /* 色の範囲に関する変数 */
    	double symratio;  /* 左右対称度 */

	//printf("\n処理３と４：領域ごとの特徴を判定します\n");
    	/* ラベルNo.n の領域を順番に調べます */

        /* 各変数の初期化 */
        	
        /* 範囲の初期化 */
        xmin=input->cols;   xmax=0;
        ymin=input->rows;  ymax=0;
        
        /* 面積の初期化 */
        sum_area=0;
        
        /* 色の範囲の初期化 */
        for(j=0;j<3;j++){
            rgbmin[j]=255;
            rgbmax[j]=0;
        }        	
        /* ラベル(大域変数 label[][])を走査します */
	for(k=0;k<input->rows;k++){			//ラベル番号nの面積を計算する
		for(j=0;j<input->cols;j++){
			if(lab[k][j]==n){
			/* 面積（領域）の更新 */
				sum_area++;	
			
				/* 左上と右下の点の更新 */
                   		if ( j < xmin ) xmin = j;
                   		if ( j > xmax ) xmax = j;
                   		if ( k < ymin ) ymin = k;
                    		if ( k > ymax ) ymax = k;
                    			
                    		/*色の範囲の更新*/
				if(input->p[k][j].r<rgbmin[0])		// R成分
					rgbmin[0]=input->p[k][j].r;
				if(input->p[k][j].r>rgbmax[0])
					rgbmax[0]=input->p[k][j].r;
				
				if(input->p[k][j].g<rgbmin[1])		//G成分
					rgbmin[1]=input->p[k][j].g;
				if(input->p[k][j].g>rgbmax[1])
					rgbmax[1]=input->p[k][j].g;
				
				if(input->p[k][j].b<rgbmin[2])		//B成分
					rgbmin[2]=input->p[k][j].b;
				if(input->p[k][j].b>rgbmax[2])
					rgbmax[2]=input->p[k][j].b;							
                    	} /*if文終了*/
		}
	} /*ラベル走査終了*/		

	/* 1) 面積の画像全体に対する比率＞AREARATIO の確認 */
        if ( (double)(sum_area)/input->cols/input->rows >= AREARATIO ){
        	printf("=== No.%d ===\n",n);
        	printf("  画像における面積率の条件をクリア：");
         	printf("面積率＝%f\n",(double)(sum_area)/input->cols/input->rows);
            	W = xmax - xmin + 1;
            	H = ymax - ymin + 1;
            	
            	/* 2) 外接四角形における面積率>AREARATE の確認 */
            	if ( ( sum_area / ( W * H ) ) > AREARATE ){
                	printf("  外接四角形における面積率の条件をクリア：");
                	printf("四角形面積率＝%f\n",sum_area/(W*H));
                	
                	/* 3) 縦横比の計算 */
                	if ( ( H/W > HWMIN ) && ( H/W < HWMAX )){
                    		printf("  縦横比の条件をクリア：");
                    		printf("縦横比＝%f\n",H/W);
                    	
                    		/* 4) 色の範囲の検査 */
                    		colrange=0;
                        	colrange = (rgbmax[0] - rgbmin[0]) + (rgbmax[1] - rgbmin[1]) + (rgbmax[2] - rgbmin[2]);
                    		if ( colrange > 250 ){
                        		printf("  色の範囲の条件もクリア：");
                        		printf("色の範囲を表す量：%d\n",colrange);
                        		
                        		/* 5) 左右対称性の検査 */
                        		symratio = symmetry( input, xmin, ymin, xmax, ymax );
                        		if ( symratio < SYMMAX ){
                        	    		printf("  左右対称度の条件もクリア：");
                        	    		printf("左右対称度＝%f\n",symratio);
                        	    		printf("★ No.%d の領域を顔の候補と判定します★\n\n",n);                        	    		
                        	    		printf("\t\tラベル番号->%2d\t\t面積は%d\t(%lf％)\n\n",n,sum_area,((double)sum_area/(input->rows*input->cols))*100.0);
                        	    		
                        	    		return n;	// 1) ~ 5)の条件を全て満たす場合、ラベル番号を顔領域としてラベル番号を返す
                   	    		}
        	 		}
                	}	
            	}
	}
	else{
		return 0;
	}
	printf("◆ No.%d の領域は顔の候補の条件をクリアできませんでした（顔候補から削除）◆\n\n",n);
	return 0;	
}
/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊こ　こ　ま　で＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/


/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊画像をすべて白色にする関数＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/

void init(RGB_PACKED_IMAGE *input){	
	int k,l;
	for(l=0;l<input->rows;l++){
		for(k=0;k<input->cols;k++){
			input->p[l][k].r=255;
			input->p[l][k].g=255;
			input->p[l][k].b=255;
		}	
	}
}

/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊こ　こ　ま　で＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/


/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊顔領域のみを抽出する関数＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/

void face(RGB_PACKED_IMAGE *input,RGB_PACKED_IMAGE *output,int lab[input->rows][input->cols],int n){
	int k,l;
	for(l=0;l<input->rows;l++){
		for(k=0;k<input->cols;k++){
			if(lab[l][k]==n){
				output->p[l][k].r=input->p[l][k].r;
				output->p[l][k].g=input->p[l][k].g;
				output->p[l][k].b=input->p[l][k].b;
			}
			else{
				if(input->p[l][k].r!=255&&input->p[l][k].g!=255&&input->p[l][k].b!=255){	/*何もしない*/	}
				else{
					output->p[l][k].r=255;
					output->p[l][k].g=255;
					output->p[l][k].b=255;
				}	
			}	
		}
	}
}
/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊こ　こ　ま　で＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/


/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊領域の左上端,右下端を判定する関数＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/
void edge(RGB_PACKED_IMAGE *input,int lab[input->rows][input->cols],int num,int ul[2],int lr[2]){
	int j,k;						//ul[0]=左上のx座標,ul[1]=左上のy座標
	int flag=0;						//lr[0]=右下のx座標 lr[1]=右下のy座標
	ul[0]=0;lr[0]=0;
	ul[1]=0;lr[1]=0;
	for(k=0;k<input->rows;k++){				//左上のy座標の決定
		for(j=0;j<input->cols;j++){
			if(lab[k][j]==num){
				if(flag==0){
					ul[1]=k;
					flag=1;
				}
			}
		}
	}

	flag=0;

	for(k=0;k<input->cols;k++){				//左上のx座標の決定
		for(j=0;j<input->rows;j++){
			if(lab[j][k]==num){
				if(flag==0){
					ul[0]=k;
					flag=1;
				}
			}
		}
	}
	flag=0;
	for(k=input->rows-1;k>=0;k--){				//右下のy座標の決定
		for(j=0;j<input->cols;j++){
			if(lab[k][j]==num){
				if(flag==0){
					lr[1]=k;
					flag=1;
				}
			}
		}
	}
	flag=0;
	for(k=input->cols-1;k>=0;k--){				//右下のx座標の決定
		for(j=0;j<input->rows;j++){
			if(lab[j][k]==num){
				if(flag==0){
					lr[0]=k;
					flag=1;
				}
			}
		}
	}

	if((double)(lr[1]-ul[1])/(lr[0]-ul[0])>FACE_RATE){	//顔の縦横比を補正する
		printf("\t＜＜＜＜＜ラベル番号->%dの顔の縦横比が異常です!!!!＞＞＞＞＞\n",num);
		
		lr[1]=ul[1]+(int)(FACE_RATE*(lr[0]-ul[0]));

		printf("\t\t________顔の縦横比を修正しました________\t\n\n");
	}
	printf("ラベル番号%dの領域の左上の座標は(%d,%d),右下の座標は(%d,%d)です\n\n",num,ul[0],ul[1],lr[0],lr[1]);
}								//メッセージ出力

/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊こ　こ　ま　で＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/


/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊四角形を書く関数＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/
void draw_rect(RGB_PACKED_IMAGE *input,int x1,int y1,int x2,int y2,int bold){
	int i,j;

	//水平方向に直線を書く	
	for(j=x1;j<=x2;j++){
		for(i=-bold/2;i<=bold/2;i++){
			if(0<=y1+i){	
				input->p[y1+i][j].r=0;
				input->p[y1+i][j].g=255;
				input->p[y1+i][j].b=0;
			}
		}
	}
	for(j=x1;j<=x2;j++){
		for(i=-bold/2;i<=bold/2;i++){
			if(input->rows>y2+i){
				input->p[y2+i][j].r=0;
				input->p[y2+i][j].g=255;
				input->p[y2+i][j].b=0;
			}
		}
	}

	//垂直方向に直線を書く
	for(j=y1;j<=y2;j++){
		for(i=-bold/2;i<=bold/2;i++){
			if(0<=x1+i){	
				input->p[j][x1+i].r=0;
				input->p[j][x1+i].g=255;
				input->p[j][x1+i].b=0;
			}
		}
	}
	for(j=y1;j<=y2;j++){
		for(i=-bold/2;i<=bold/2;i++){
			if(input->cols>x2+i){	
				input->p[j][x2+i].r=0;
				input->p[j][x2+i].g=255;
				input->p[j][x2+i].b=0;
			}
		}
	}
}

/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊こ　こ　ま　で＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/


/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊モザイク型の四角形を書く関数＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/
void draw_rect_mosaic(RGB_PACKED_IMAGE *input,int x1,int y1,int x2,int y2,int bold){
	int i,j;
	int rr, gg, bb;
	int s = 5;
	int Y1, Y2;
	
	
/*	if(x2-x1>110 || y2-y1 > 150){
		Y1 = y1 + 50;
		Y2 = y1 + 75;
	}
	else{
		Y1 = y1 + 5;
		Y2 = y1 + 30;
	}
*/

	if(y2 - y1 > 75){
		Y1 = (int)( y1 + (y2 - y1) / 5);
		Y2 = Y1 + 15;
	}
	else{		// 顔の枠が小さい場合
		Y1 = (int)( y1 + (y2 - y1) / 7);
		Y2 = Y1 + 6;
	}	
	
	for(j=x1;j<=x2;j++){
		for(i=Y1;i<=Y2;i++){
			rr=input->p[i][j].r;
			gg=input->p[i][j].g;
			bb=input->p[i][j].b;
		
			rr=rr/(s*s)+(rand() % 96)-48; // // 各ピクセルの色を平均し-48～48の乱数加算
  			gg=gg/(s*s)+(rand() % 96)-48;
  			bb=bb/(s*s)+(rand() % 96)-48;

			input->p[i][j].r=(BYTE)rr; // ブロックの色計算
			input->p[i][j].g=(BYTE)gg;
			input->p[i][j].b=(BYTE)bb;
			  
			rr = 0; gg = 0; bb = 0;
		}
	}
}

/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊こ　こ　ま　で＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/


/*＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊		メイン処理		＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊*/

#ifdef __STDC__
int
main( int argc, char *argv[] )
#else
int
main( argc, argv )
     int argc ;
     char *argv[] ;
#endif
{

	char *name_img_in = "image3.ppm" ;  			/* 入力画像ファイル名 */
	char *name_img_out_1 = "out_Flesh_color_area.ppm" ;  	/* 肌色領域の画像ファイル名 */
	char *name_img_out_2 = "out_face_area.ppm" ;  		/* 顔画像ファイル名 */
	char *name_img_out_3 = "out_result_detect.ppm" ;  	/* 顔抽出結果画像ファイル名 */
	char *name_img_out_4 = "out_result_mosaic.ppm" ;  	/* モザイク処理結果画像ファイル名 */

	RGB_PACKED_IMAGE *image_in ;	/* カラー画像用構造体(顔抽出用) */
	RGB_PACKED_IMAGE *image_in_1 ;	/* カラー画像用構造体（モザイク加工用） */
	RGB_PACKED_IMAGE *image_Skin_P ;/*肌色画素検出用*/
	RGB_PACKED_IMAGE *image_face ;	/*結果出力画像用*/

	/* コマンドラインでファイル名が与えられた場合の処理 */
	if ( argc == 1 ){
		printf("■■■\t\t入力画像をコマンドラインで入力してください。\t\t■■■\n");
		printf("■■■\t現在はファイル名：%sが入力画像に設定されています\t■■■\n\n\n",name_img_in);
	}
	if ( argc >= 2 ) name_img_in = argv[1] ;
	if ( argc >= 3 ) name_img_out_1 = argv[2] ;

	/* 入力画像ファイルのオープンと画像データ獲得 */
	if (!( image_in = readRGBPackedImage( name_img_in ))) {
		printError( name_img_in ) ;
		return(1) ;
	}
  
	/* 入力画像ファイルのオープンと画像データ獲得 */
	if (!( image_in_1 = readRGBPackedImage( name_img_in ))) {
    		printError( name_img_in ) ;
    		return(1) ;
  	}  

	/*各種構造体や変数の宣言*/
	image_Skin_P = allocRGBPackedImage(image_in->cols,image_in->rows);	//肌色画素検出用
 	image_face = allocRGBPackedImage(image_in->cols,image_in->rows);	//結果出力画像用
 	int label[image_in->rows][image_in->cols];	//ラベル配列
 	int UL[2],LR[2];		//領域の端を格納する配列
 	int a,cnt=0,temp=0;		//カウンター
 	int face_label_num[NOP]={0};	//顔の候補の領域のラベル番号を格納する配列
 	int SOP=0;	//検出できた人数をカウント用
	
	/*処理開始*/	
	process_no1(image_in,image_Skin_P);	//入力画像から肌色画素を抽出する

	int number = labeling(image_Skin_P,255,255,255,label);		//肌色領域の数を調べるそれを、numeberに代入

	if ( writeRGBPackedImage( image_Skin_P, name_img_out_1 ) == HAS_ERROR ) {		//肌色領域をすべて画像ファイルに出力
	    printError( name_img_out_1 ) ;					
	    return(1) ;
	}
	printf("\n＊＊＊\t\t肌色領域をラベリングした結果を出力しました\t＊＊＊\n＊＊＊\t\tファイル名：%s\t\t＊＊＊\n\n",name_img_out_1);									

	printf("***************************\t顔の候補\t*****************************\n");	//メッセージ出力

	for(a=1;a<=number;a++){							//顔候補の領域のラベル番号を調べる。
		temp=cal_area_label(image_Skin_P,label,a);
		if(temp!=0){
			if(cnt==NOP){}
			else{
				face_label_num[cnt]=temp;			//顔候補の領域のラベル番号を代入
				cnt++;
			}
		}
	
	}
	printf("*****************************************************************************\n");	//メッセージ出力

	init(image_face);							//出力画像の初期化
	
	for(a=0;a<NOP;a++){
		if(face_label_num[a]!=0){
			face(image_Skin_P,image_face,label,face_label_num[a]);	//顔候補の領域を書き込む
		}
	}
	printf("\n＊＊＊\t\t\t顔候補の領域を出力しました\t\t＊＊＊\n＊＊＊\t\t\tファイル名：%s\t\t＊＊＊\n\n",name_img_out_2);	
										//メッセージ出力
	
	for(a=0;a<NOP;a++){							//顔候補の領域の端を調べる。
		if(face_label_num[a]!=0){
			SOP++;
			edge(image_Skin_P,label,face_label_num[a],UL,LR);
			draw_rect(image_in,UL[0],UL[1],LR[0],LR[1],4);
			draw_rect_mosaic(image_in_1,UL[0],UL[1],LR[0],LR[1],4);
		}
	}
	printf("\n＊＊＊\t\t\t%d人分の顔の判定を判定できました\t\t＊＊＊\n",SOP);


	if ( writeRGBPackedImage( image_face, name_img_out_2 ) == HAS_ERROR ) {		//顔候補の領域を画像ファイルに出力
		printError( name_img_out_2 ) ;
		return(1) ;
	}


	/* 出力画像ファイルのオープンと画像データの書込み */

   	if ( writeRGBPackedImage( image_in, name_img_out_3 ) == HAS_ERROR ) {		//最終結果を出力
    		printError( name_img_out_3 ) ;
    		return(1) ;
  	}
  	
  	printf("\n＊＊＊\t\t\t顔抽出結果の画像を出力しました\t\t＊＊＊\n＊＊＊\t\t\tファイル名：%s\t\t＊＊＊\n\n",name_img_out_3);	
		//メッセージ出力
  	
  	if ( writeRGBPackedImage( image_in_1, name_img_out_4 ) == HAS_ERROR ) {		//最終結果を出力
    		printError( name_img_out_3 ) ;
    		return(1) ;
  	}
  	
 	printf("\n＊＊＊\t\t\tモザイク加工結果の画像を出力しました\t\t＊＊＊\n＊＊＊\t\t\tファイル名：%s\t\t＊＊＊\n\n",name_img_out_4);	
		//メッセージ出力

  	return(0);

}
