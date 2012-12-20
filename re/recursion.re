= 再帰

この章では再帰について学びます。ターゲットとなるプログラムは次のものです。

//emlist{
[:letrec, 
 [[:fact,
   [:lambda, [:n], [:if, [:<, :n, 1], 1, [:*, :n, [:fact, [:-, :n, 1]]]]]]], 
 [:fact, 3]]
//}

まずは準備として、μSchemeRの機能を少し拡張しましょう。


== 条件式

次のようなif式で条件を扱えるようにします。

//emlist{
[:if, [:>, 3, 2], 1, 0]
//}

if式の評価はif式から条件、真節、偽節を取得し、条件の評価値が真であれば真節を評価し、偽であれば偽節を評価し、その値を返します。

====[column] ifが関数だと?

if式をspecial formとして評価していますが、組み込み関数としなかったのはなぜでしょう。

我々の言語では、関数は、引数をすべて評価してから関数適用を行うため、条件が真であっても偽節が評価されてしまうのです。実行速度が遅くなるのもそうですが、副作用があるときに問題となります。xが1のとき、代入文set!を使った(第5章でも説明します)次の式を評価した後のxの値は4になります。これは、真節の式も評価するためです。

//emlist{
[:if, :false, 
  [:set!, :x, [:+, :x, 1], 
  [:set!, :x, [:+, :x, 2]]
//}

====[/column]

if式を評価するプログラムを上のとおり書いていきましょう(後で使う@<tt>{letrec}も合わせて一緒に定義しています)。

//emlist{
def special_form?(exp)
  lambda?(exp) or 
     let?(exp) or 
     letrec?(exp) or 
     if?(exp)
end

def eval_special_form(exp, env)
  if lambda?(exp)
    eval_lambda(exp, env)
  elsif let?(exp)
    eval_let(exp, env)
  elsif letrec?(exp)
    eval_letrec(exp, env)
  elsif if?(exp)
    eval_if(exp, env)
  end
end

def eval_if(exp, env)
  cond, true_clause, false_clause = if_to_cond_true_false(exp)
  if _eval(cond, env)
    _eval(true_clause, env)
  else
    _eval(false_clause, env)
  end
end

def if_to_cond_true_false(exp)
  [exp[1], exp[2], exp[3]]
end

def if?(exp)
  exp[0] == :if
end
//}

if式で分岐するために論理値のリテラルを導入します。 @<tt>{:true}, @<tt>{:false}は(Rubyの)true, falseとして解釈するよう大域環境に加えます。

//emlist{
$boolean_env = 
    {:true => true, :false => false}
$global_env = [$primitive_fun_env, $boolean_env]
//}

条件式で扱えるよう、組み込み関数に不等号、等号の演算子を加えます。

//emlist{
$primitive_fun_env = {
  :+  => [:prim, lambda{|x, y| x + y}],
  :-  => [:prim, lambda{|x, y| x - y}],
  :*  => [:prim, lambda{|x, y| x * y}],
  :>  => [:prim, lambda{|x, y| x > y}],
  :>= => [:prim, lambda{|x, y| x >= y}],
  :<  => [:prim, lambda{|x, y| x <  y}],
  :<= => [:prim, lambda{|x, y| x <= y}],
  :== => [:prim, lambda{|x, y| x == y}],
}
$global_env = [$primitive_fun_env, $boolean_env]
//}


== 再帰

これで準備ができました。いよいよ再帰を見ていきます。次のプログラムを実行してみましょう。

//emlist{
[:let, 
 [[:fact,
   [:lambda, [:n], [:if, [:<, :n, 1], 1, [:*, :n, [:fact, [:-, :n, 1]]]]]]], 
 [:fact, 0]]
//}

1が表示されましたか。では次のプログラムはどうでしょう。

//emlist{
[:let, 
 [[:fact,
   [:lambda, [:n], [:if, [:<, :n, 1], 1, [:*, :n, [:fact, [:-, :n, 1]]]]]]], 
 [:fact, 1]]
//}

エラーになりました。この違いは何でしょう。@<tt>{:let}を評価すると@<tt>{:fact}をλ式の値であるクロージャに束縛します。このクロージャの環境には@<tt>{:fact}は含まれていない点に注意しましょう。@<tt>{[:fact 1]}として関数適用するとクロージャ中の環境を用いて評価します。評価を進め、if式で偽となると@<tt>{:fact}を評価しますが、先に述べたようにこの環境には@<tt>{:fact}は含まれていないためエラーとなるのです(@<img>{let})。すなわち、関数として定義しようとした式の中でその関数の名前を使うにはlet式では不十分であることが分かります。

//image[let][[:fact 1\]評価時の環境のようす。λ式評価時の環境にfactがないため、クロージャ内の環境にはfactは存在せず、[:fact 1\]でクロージャ中のfactを参照しようとするとエラーになる。]{
//}

これを解決するためにはどうすれば良いでしょう。問題は、先に述べたように、@<tt>{:lambda}を評価してクロージャを作るときに@<tt>{:fact}が束縛されていないため、返されたクロージャを関数適用に用いると、その中の@<tt>{:fact}が評価できない点にあります。すなわち、作られるクロージャ中の環境に、それができた後で束縛しようとしている@<tt>{:fact}が含まれている必要があるのです。

これを解決するために、少しトリッキーなことを行います。アイデアは、λ式を評価しクロージャを作成するときに環境として予め、パラメータ(ここでは@<tt>{:fact})の領域を確保しておくことです。ただし、それを束縛する値はまだ定まっていないので、ダミーの値を入れておきます(@<img>{letrec}(a))。このとき、パラメータは評価されないためダミーの値でも問題にはなりません。λ式を評価してもλ式の中は評価されずにクロージャとして返されるためです。λ式の評価値であるクロージャが得られたら、先ほどのパラメータをダミーの値からその値に束縛するように変更します。(@<img>{letrec}(b))これでパラメータを評価すると、そのパラメータを環境として含むクロージャを得ることができます。得られた環境を使ってletrec式のボディを評価すると(@<img>{letrec}(c))、λ式内の@<tt>{:fact}を所望どおり参照することができます。

//image[letrec][(a) λ式の評価時の環境にfact をダミー値dummy に束縛しておき、(b) 得られたクロージャの環境のfactをクロージャ自身に束縛することで、(c)[:fact 1\] でクロージャの環境内のfact が参照可能となる]{
//}

以上の機構を実装した再帰を扱う@<tt>{letrec}を導入します。

//emlist{
def eval_letrec(exp, env)
  parameters, args, body = letrec_to_parameters_args_body(exp)
  tmp_env = Hash.new
  parameters.each do |parameter| 
    tmp_env[parameter] = :dummy
  end
  ext_env = extend_env(tmp_env.keys(), tmp_env.values(), env)
  args_val = eval_list(args, ext_env)
  set_extend_env!(parameters, args_val, ext_env)
  new_exp = [[:lambda, parameters, body]] + args
  _eval(new_exp, ext_env)
end

def set_extend_env!(parameters, args_val, ext_env)
  parameters.zip(args_val).each do |parameter, arg_val|
    ext_env[0][parameter] = arg_val
  end
end

def letrec_to_parameters_args_body(exp)
  let_to_parameters_args_body(exp)
end

def letrec?(exp)
  exp[0] == :letrec
end
//}

それでは、実際に試してみましょう。

//emlist{
exp =
  [:letrec, 
   [[:fact,
     [:lambda, [:n], [:if, [:<, :n, 1], 1, [:*, :n, [:fact, [:-, :n, 1]]]]]]], 
   [:fact, 3]]
puts _eval(exp, $global_env)
//}

正しく、6が表示されましたね。


== 純粋関数型言語 – 代入はどこへ?

おめでとうございます。あなたは、この時点で関数型言語で必要な全ての要素を学んだと言えます。ただし「最小限の範囲内で」、という限定付きです。

今までに学んできたプログラミング言語は、純粋関数型言語(pure functional language)と言われ、代入と言った副作用の無い言語です。状態が変わらない、そもそも状態すら持たないために、変数を、それを束縛している値で入れ替えても問題ありません。最適化ではインライン展開と言われる手法です。関数呼び出しを減らすことができるためそれに関わるコストを削減できます。関数呼び出し時に、環境のスタックを積み上げたことを思い出して下さい。それが不要になります。もしくは、どの順序で評価してもその値は変わらないため、それぞれを並列に処理することができます。最近、関数型言語が見直されるようになってきたのは、このような背景が多分にあります。

そうは言っても手続き型言語で代入を多用している方にとっては、本当に代入なしで複雑な処理を記述できるのか疑問に思われることでしょう。大丈夫です。実は、今まで書いてきたRubyのプログラムも簡単に代入なしで書くことができます。本質的に代入が必要なのは、たった一ヶ所、set_extend_env!で使われているハッシュへの代入のみです@<fn>{fn1}。他の箇所では引数で渡された配列の中を代入などにより変更していません。したがってこれ以外の代入は、let式相当の機能で置き換えても問題ありません(Rubyにはlet式相等の機能がありませんが……)。

本書で考えるプログラミング言語にはあえて代入は導入しません。下手に代入があるとそれに頼ってしまうことがあるからです。新しい関数型言語の考え方に慣れるためにこの言語で色々と処理を書いて見てください。


== 関数型言語と再帰 – for文はどこへ?

再帰は関数型言語にとって大きな意味を持ちます。手続き型言語のfor文などループに相当する重要な制御構文です。関数型言語の特徴は、データ構造に着目して再帰的に処理をすることが多い点です。

実は今回の対象としているプログラミング言語も、数字など基本的な式を組み合わせて一つのプログラムとなるように設計されています。これを式の構成に関する帰納法と呼びます。eval_letなどの中で、評価すべきプログラムを構成している要素に分け、それぞれの要素に対して再帰的にevalしていることを確認してみて下さい。

また、先の例では階乗を求めるプログラムfactを使いました。これも見方を変えると再帰的に定義された整数に従った構成に関する帰納法とも言えます。整数は、0という整数が存在すること、またある整数が存在したとき+1したものが次に大きな整数として定義されます。この定義に従い、0のときの解を示し、またある整数が与えられた時、それより一つ小さい整数の解を用いて求めるものを定義することで、全ての場合の解を求めることができます。

その他にもまだ扱っていませんが、リストは空リストであるか、リストに先頭の要素を加えたものである、と再帰的に定義されます。一般的なリストを扱う関数はこの定義に従い、与えられたリストが空リストであった場合の処理と、与えられたリストの最初の要素に対する処理を記載し、残りのリストは再帰を使って定義します。わかりづらいと思いますので、「4.1 リスト」の項目(特にlength関数)を見てからもう一度この文章を読んで見てください。


== まとめ

この章では次のことを学びました。

 * 再帰関数を実現するための方法
 * 再帰関数の評価値であるクロージャは、その中の環境でクロージャ自身を参照する。
 * これまでに作成してきたμSchemeRは代入のような副作用がない純粋関数型言語である
 * 手続き言語のループに相当するものを関数型言語では再帰を用いる

//footnote[fn1][関数名の最後の!は、quoteの 略で、Schemeでは一般的に副作用がある関数の最後にクオート文字!をつけてこ れを区別しています。この慣習に倣いました。]


