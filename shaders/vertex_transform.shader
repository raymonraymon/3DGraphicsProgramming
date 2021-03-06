// `設定頂點的資料格式`
struct VS_INPUT
{
	float4 Position : POSITION;
};

// `設定Vertex Shader輸出的資料格式`
struct VS_OUTPUT
{
	float4 Position : POSITION;
};

// `轉換矩陣`
uniform row_major float4x4 viewproj_matrix : register(c0);
uniform float4x4 viewproj_matrix_t : register(c0);

// Vertex Shader
VS_OUTPUT VS(VS_INPUT In)
{
	VS_OUTPUT Out;
	// `座標轉換`
	Out.Position = mul( In.Position, viewproj_matrix);
	// `不把矩陣視為 row major 時, 乘法要反過來做.`
	// Out.Position = mul( viewproj_matrix_t, In.Position );
	
	return Out;
}

// Pixel Shader
float4 PS(VS_OUTPUT In) : COLOR
{
	// `傳回白色, 永遠畫一個白點.`
	return float4(1,1,1,1);
}
