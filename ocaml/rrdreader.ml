(*
 * Copyright (c) 2016 Citrix
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *)

(* This is a small utility to test the librrd library *)

open Rrd_protocol

exception Error of string
let error fmt  = Printf.kprintf (fun msg -> raise (Error msg)) fmt

let finally opn cls =
	let res = try opn () with exn -> cls (); raise exn in
	cls ();
	res

let string_of_data_source owner ds =
	let owner_string = match owner with
	| Rrd.Host -> "Host"
	| Rrd.SR sr -> "SR " ^ sr
	| Rrd.VM vm -> "VM " ^ vm
	in
	let value_string = match ds.Ds.ds_value with
	| Rrd.VT_Float f -> Printf.sprintf "float %f" f
	| Rrd.VT_Int64 i -> Printf.sprintf "int64 %Ld" i
	| Rrd.VT_Unknown -> Printf.sprintf "unknown"
	in
	let type_string = match ds.Ds.ds_type with
	| Rrd.Absolute  -> "absolute"
	| Rrd.Gauge     -> "gauge"
	| Rrd.Derive    -> "derive"
	in
	Printf.sprintf
		"owner: %s\nname: %s\ntype: %s\nvalue: %s\nunits: %s"
		owner_string ds.Ds.ds_name type_string value_string ds.Ds.ds_units

let print payload =
	print_endline "------------ Metadata ------------";
	Printf.printf "timestamp = %Ld\n%!" payload.timestamp;
	print_endline "---------- Data sources ----------";
	List.iter
		(fun (owner, ds) ->
			print_endline (string_of_data_source owner ds);
			print_endline "----------")
		payload.datasources

let read path =
	let v2 = Rrd_protocol_v2.protocol in
	let reader = Rrd_reader.FileReader.create path v2 in
	finally
		(fun () ->
			reader.Rrd_reader.read_payload ()
			|> print)
		(fun () ->
			reader.Rrd_reader.cleanup ())

let read_loop seconds path =
	let v2 = Rrd_protocol_v2.protocol in
	let reader = Rrd_reader.FileReader.create path v2 in
	let rec loop () =
		( reader.Rrd_reader.read_payload () |> print
		; Unix.sleep seconds
		; loop ()
		) in
	finally loop reader.Rrd_reader.cleanup

let atoi str =
	try int_of_string str with Failure _ -> error "not an int: %s" str

let main () =
	let args   = Array.to_list Sys.argv in
	let this   = Sys.executable_name in
	let printf = Printf.printf in
		match args with
		| [_;path]        -> read path; exit 0
		| [_;"-l";s;path] -> read_loop (atoi s) path
		| _               -> printf "usage: [-l secs] %s file.rrd\n" this; exit 1

let () = main ()


