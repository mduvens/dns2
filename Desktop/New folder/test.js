let a = {
    0: 1,
    1:2,
    2:{
        0:1
    }
}
let b = {
    0: "one",
    1:"two",
    2:{
        0:"one"
    }
}

b[2]= a[2][0]
// b[2][0] = 5
// console.log(b[2])

let c ="abc"
let d;
d = c;
c ="xyz"
console.log(d)